// Pure CPU request reducer. Callers serialize access; no lock crosses game/GPU work.
#pragma once
#include "../common/fsr4_quality.h"
#include "game_render_scale_binding.h"
#include "fsr4_dispatch_status.h"
#include "vr_ssaa_hold_math.h"
#include <cstdio>
#include <cstring>
namespace edvr {
namespace renderscale {
enum class State { Game, PendingGame, PendingBoundary, AwaitingFsr, Applied, Refused, Restoring };
struct RequestIntent {
    uint64_t sequence = 0, profileGeneration = 0;
    bool vr = false, fsrEnabled = false;
    fsr4_quality::Request quality{};
};
// Config owns the requested AA mode, including runtime overrides. Keep one coherent
// read path for the real frame publisher and the real-Config regression rig.
template<class Reader>
inline bool readRequestIntent(Reader& reader, bool vr, bool featuresAllowed, RequestIntent& result) {
    const auto generation = reader.generation();
    RequestIntent candidate;
    if (!fsr4_quality::readRequest(reader, candidate.quality)) return false;
    const auto mode = reader.requestedTemporalMode();
    if (generation != reader.generation()) return false;
    candidate.vr = vr;
    candidate.fsrEnabled = featuresAllowed && fsr4_quality::controlsEnabled(mode);
    result = candidate;
    return true;
}
struct OutputBasis {
    unsigned width = 0, height = 0;
    bool settled = false;
};
struct PlannedAction {
    bool valid = false, restoring = false, releasing = false;
    uint64_t id = 0, requestSequence = 0, ownerGeneration = 0, epoch = 0;
    ScaleAction action{};
    unsigned inW = 0, inH = 0, outW = 0, outH = 0;
};
struct Snapshot {
    State state = State::Game;
    RequestIntent requested{};
    uint64_t revision = 0, ownerGeneration = 0, epoch = 0;
    unsigned expectedInW = 0, expectedInH = 0, expectedOutW = 0, expectedOutH = 0;
    bool actualValid = false;
    LiveOwner actual{};
    Fsr4DispatchStatus slots[3]{};
    unsigned mismatches[3]{};
    char reason[512] = {};
};
class ScaleController {
public:
    void publishRequest(const RequestIntent& r) {
        if (closed_ || r.sequence <= s_.requested.sequence) return;
        const auto& previous = s_.requested;
        const bool sameChoice = r.profileGeneration == previous.profileGeneration && r.vr == previous.vr &&
            r.fsrEnabled == previous.fsrEnabled && r.quality.valid == previous.quality.valid &&
            r.quality.preset == previous.quality.preset &&
            (r.quality.preset != fsr4_quality::Preset::Custom || r.quality.customScale == previous.quality.customScale);
        if (sameChoice && s_.state == State::Applied) {
            s_.requested = r;
            ++s_.revision;
            return;
        }
        const bool release = r.profileGeneration != s_.requested.profileGeneration ||
        r.vr != s_.requested.vr || !r.fsrEnabled || r.quality.preset == fsr4_quality::Preset::Game;
        s_.requested = r;
        failedRequest_ = false;
        releaseOnly_ = false;
        invalidate();
        callbackSeen_ = false;
        boundaries_ = 0;
        if (release && (receipt_ || inFlight_.valid)) {
            releasing_ = true;
            restoring_ = true;
        }
        if (restoring_ && releasing_) {
            set(State::Restoring, "releasing previous game scale");
            return;
        }
        if (!r.quality.valid) {
            set(State::Refused, "invalid render quality configuration");
            return;
        }
        if (restoring_) {
            set(State::Restoring, "releasing previous game scale");
            return;
        }
        start();
    }
    void setOutput(OutputBasis output) {
        if (closed_ || (output.width == output_.width && output.height == output_.height &&
            output.settled == output_.settled)) return;
        output_ = output;
        invalidate();
        if (!restoring_ && s_.requested.quality.valid) start();
    }
    // Fresh settings-copy values are genuine unless they still equal our writes.
    void observeGameSettings(const LiveOwner& owner) {
        if (closed_) return;
        adopt(owner);
        if (!receipt_) baseline_ = owner;
        else {
            bool changed = (ssOwned_ && owner.ss != last_.ss) || (hmdOwned_ && owner.hmd != last_.hmd) ||
            (managerOwned_ && owner.managerHmd != last_.managerHmd);
            if (!ssOwned_ || owner.ss != last_.ss) {
                baseline_.ss = owner.ss;
                ssOwned_ = false;
            }
            if (!hmdOwned_ || owner.hmd != last_.hmd) {
                baseline_.hmd = owner.hmd;
                hmdOwned_ = false;
            }
            if (!managerOwned_ || owner.managerHmd != last_.managerHmd) {
                baseline_.managerHmd = owner.managerHmd;
                managerOwned_ = false;
            }
            if (changed && !restoring_) {
                invalidate();
                start();
            }
        }
        updateActual(owner);
    }
    float flatSetterValue(const LiveOwner& live, float requested, float held) {
        if (closed_ || !haveOwner_ || !sameOwner(live, owner_) || live.mode != 0 ||
            !receipt_ || !ssOwned_ || live.ss != last_.ss || restoring_ || releaseOnly_ ||
            !s_.requested.fsrEnabled || !s_.requested.quality.valid || s_.requested.vr ||
            s_.requested.quality.preset == fsr4_quality::Preset::Game) return held;
        if (std::isfinite(requested) && std::isfinite(live.ssMin) && std::isfinite(live.ssMax) &&
            live.ssMin >= 0 && live.ssMax >= live.ssMin)
            baseline_.ss = std::clamp(requested, live.ssMin, live.ssMax);
        return last_.ss;
    }
    PlannedAction planGameAction(const LiveOwner& owner) {
        if (closed_ || inFlight_.valid) return {};
        adopt(owner);
        callbackSeen_ = true;
        if (!s_.requested.quality.valid && !(restoring_ && releasing_)) return {};
        updateActual(owner);
        if (restoring_) return planRestore(owner);
        if (s_.state != State::PendingGame || !s_.requested.quality.valid) return {};
        if (!ssaahold::blockModeKnown(owner.mode)) {
            refuse("unknown game render mode");
            return {};
        }
        if ((owner.mode != 0) != s_.requested.vr) {
            set(State::PendingGame, "game/profile mode mismatch");
            return {};
        }
        if (!output_.settled || !output_.width || !output_.height) {
            set(State::PendingGame, "awaiting settled FSR output");
            return {};
        }
        float scale = fsr4_quality::scaleFor(s_.requested.quality.preset, s_.requested.quality.customScale);
        unsigned w = 0, h = 0;
        if (!std::isfinite(scale) || scale < fsr4_quality::minimumCustomScale || scale > 1 ||
            !fsr4_quality::scaledDimension(output_.width, scale, w) ||
            !fsr4_quality::scaledDimension(output_.height, scale, h)) {
            refuse("invalid managed render scale");
            return {};
        }
        if (owner.mode == 0 && (!std::isfinite(owner.ssMin) || !std::isfinite(owner.ssMax) ||
            owner.ssMin < 0 || owner.ssMax < owner.ssMin || scale < owner.ssMin || scale > owner.ssMax)) {
            refuse("requested scale outside valid game bounds");
            return {};
        }
        auto p = makePlan(owner, false);
        p.inW = w;
        p.inH = h;
        p.outW = output_.width;
        p.outH = output_.height;
        if (owner.mode == 0) p.action.ss = {true, owner.ss, scale};
        else {
            p.action.contextHmd = {true, owner.hmd, scale};
            p.action.managerHmd = {true, owner.managerHmd, scale};
        }
        inFlight_ = p;
        return p;
    }
    // UI preparation has made no game writes. Seeing this callback prevents a false missing-hook timeout.
    void deferGameAction(const PlannedAction& p, const char* reason) {
        if (!matches(p)) return;
        inFlight_ = {};
        if (p.requestSequence == s_.requested.sequence) set(p.restoring ? State::Restoring : State::PendingGame, reason);
    }
    // The validated callback can relinquish ownership without touching genuine new values.
    bool completeUnchangedRelease(const PlannedAction& p, const LiveOwner& live) {
        if (!matches(p) || !p.restoring || !sameOwner(p.action.owner, live) ||
            p.action.ss.write || p.action.contextHmd.write || p.action.managerHmd.write) return false;
        ApplyResult completion{};
        completion.success = true;
        completion.actualValid = true;
        completion.actual = live;
        completion.reason = "genuine game settings retained; no resize written";
        recordGameAction(p, completion);
        return true;
    }
    void recordGameAction(const PlannedAction& p, const ApplyResult& result) {
        if (!matches(p)) return;
        inFlight_ = {};
        const bool wrote = result.ssWritten || result.hmdWritten || result.managerWritten;
        if (wrote) {
            receipt_ = true;
            if (result.ssWritten) {
                last_.ss = p.action.ss.target;
                ssOwned_ = true;
            }
            if (result.hmdWritten) {
                last_.hmd = p.action.contextHmd.target;
                hmdOwned_ = true;
            }
            if (result.managerWritten) {
                last_.managerHmd = p.action.managerHmd.target;
                managerOwned_ = true;
            }
        }
        if (result.actualValid) updateActual(result.actual);
        else if (wrote && s_.actualValid) {
            s_.actualValid = false;
            ++s_.revision;
        }
        if (p.restoring) {
            restoring_ = false;
            if (releasing_ && !p.releasing && receipt_) {
                restoring_ = true;
                set(State::Restoring, "explicit release waits for rollback completion");
                return;
            }
            if (p.releasing && result.success) {
                receipt_ = ssOwned_ = hmdOwned_ = managerOwned_ = false;
                confirmed_ = false;
                releasing_ = false;
            }
            if (!result.success) {
                refuse(result.reason);
                return;
            }
            if (failedRequest_) {
                set(State::Refused, failureReason_);
                return;
            }
            start();
            return;
        }
        // The callback result is historical when UI intent changed during the unlocked write.
        if (p.requestSequence != s_.requested.sequence || p.epoch != s_.epoch) {
            invalidate();
            if (releasing_ && receipt_) {
                restoring_ = true;
                set(State::Restoring, "releasing superseded write");
            }
            return;
        }
        if (!result.success) {
            fail(result.reason, wrote);
            return;
        }
        s_.expectedInW = p.inW;
        s_.expectedInH = p.inH;
        s_.expectedOutW = p.outW;
        s_.expectedOutH = p.outH;
        invalidate();
        set(State::PendingBoundary, "game resize handed off; awaiting owned boundary");
    }
    // Only call for S_OK, non-TEST Present on the canonical device's owning thread.
    void onOwnedBoundary() {
        if (closed_) return;
        if (s_.state == State::PendingGame && !callbackSeen_ && ++boundaries_ >= 120) {
            refuse("game resize callback not observed");
            return;
        }
        if (s_.state == State::PendingBoundary) {
            invalidate();
            set(State::AwaitingFsr, "awaiting FSR frames");
        }
    }
    void observeSubmission(unsigned slot, const Fsr4DispatchStatus& d) {
        if (closed_ || slot >= 3 || (s_.state != State::AwaitingFsr && s_.state != State::Applied) ||
            d.epoch != s_.epoch || d.outcome == Fsr4DispatchOutcome::Pending ||
            !d.sequence || d.sequence <= consumed_[slot]) return;
        consumed_[slot] = d.sequence;
        const auto& previous = s_.slots[slot];
        if (previous.epoch != d.epoch || previous.outcome != d.outcome || previous.inW != d.inW ||
            previous.inH != d.inH || previous.outW != d.outW || previous.outH != d.outH ||
            std::strcmp(previous.reason, d.reason) != 0) ++s_.revision;
        s_.slots[slot] = d;
        const bool required = slot == 0 || (s_.requested.vr && slot == 1);
        if (!required) return;
        accepted_[slot] = false;
        const bool sizes = d.inW == s_.expectedInW && d.inH == s_.expectedInH &&
        d.outW == s_.expectedOutW && d.outH == s_.expectedOutH;
        if (!sizes) {
            if (++s_.mismatches[slot] >= 120) fail("FSR submitted sizes do not match requested resize", true);
            else set(State::AwaitingFsr, "awaiting matching FSR sizes");
            return;
        }
        if (d.outcome == Fsr4DispatchOutcome::Failed) {
            fail(d.reason, true);
            return;
        }
        s_.mismatches[slot] = 0;
        accepted_[slot] = true;
        if (accepted_[0] && (!s_.requested.vr || accepted_[1])) {
            confirmed_ = true;
            confirmedOwner_ = last_;
            set(State::Applied, "matching FSR frames submitted");
        } else set(State::AwaitingFsr, "awaiting both eye dispatches");
    }
    void ownerUnavailable(const char* reason) {
        if (closed_ || (!s_.actualValid && std::strcmp(s_.reason, reason ? reason : "") == 0)) return;
        invalidate();
        s_.actualValid = false;
        failedRequest_ = true;
        std::snprintf(failureReason_,sizeof(failureReason_),"%s",reason ? reason : "game owner unavailable");
        if (receipt_) {
            releasing_ = restoring_ = true;
            set(State::Restoring, reason);
        } else set(State::Refused, reason);
    }
    void requestRelease() {
        if (closed_) return;
        invalidate();
        releaseOnly_ = true;
        failedRequest_ = false;
        if (receipt_ || inFlight_.valid) {
            releasing_ = restoring_ = true;
            set(State::Restoring, "releasing game scale");
        } else set(State::Game, "");
    }
    void shutdown() {
        closed_ = true;
        invalidate();
        s_.actualValid = false;
        inFlight_ = {};
        set(State::Game, "controller shut down");
    }
    Snapshot snapshot() const {
        return s_;
    }
private:
    Snapshot s_{};
    OutputBasis output_{};
    LiveOwner owner_{}, baseline_{}, last_{}, confirmedOwner_{};
    PlannedAction inFlight_{};
    uint64_t nextAction_ = 0, consumed_[3]{};
    unsigned boundaries_ = 0;
    bool haveOwner_ = false, receipt_ = false, ssOwned_ = false, hmdOwned_ = false, managerOwned_ = false,
    confirmed_ = false, restoring_ = false, releasing_ = false, failedRequest_ = false, callbackSeen_ = false,
    closed_ = false, releaseOnly_ = false, accepted_[3]{};
    char failureReason_[512]{};
    static bool sameOwner(const LiveOwner& a, const LiveOwner& b) {
        return a.renderer == b.renderer && a.context == b.context && a.manager == b.manager && a.mode == b.mode;
    }
    void updateActual(const LiveOwner& actual) {
        if (!s_.actualValid || !sameOwner(s_.actual, actual) || s_.actual.ss != actual.ss ||
            s_.actual.hmd != actual.hmd || s_.actual.managerHmd != actual.managerHmd ||
            s_.actual.ssMin != actual.ssMin || s_.actual.ssMax != actual.ssMax) ++s_.revision;
        s_.actual = actual;
        s_.actualValid = true;
    }
    void set(State state, const char* reason) {
        const char* message = reason ? reason : "game resize failed";
        if (s_.state == state && std::strcmp(s_.reason, message) == 0) return;
        s_.state = state;
        std::snprintf(s_.reason, sizeof(s_.reason), "%s", message);
        ++s_.revision;
    }
    void invalidate() {
        ++s_.epoch;
        ++s_.revision;
        for (unsigned i = 0; i < 3; ++i) {
            accepted_[i] = false;
            consumed_[i] = 0;
            s_.mismatches[i] = 0;
        }
    }
    void refuse(const char* reason) {
        invalidate();
        set(State::Refused, reason);
    }
    void start() {
        failedRequest_ = false;
        callbackSeen_ = false;
        boundaries_ = 0;
        if (releaseOnly_) set(State::Game, "");
        else if (!s_.requested.quality.valid) set(State::Refused, "invalid render quality configuration");
        else if (!s_.requested.fsrEnabled || s_.requested.quality.preset == fsr4_quality::Preset::Game) set(State::Game, "");
        else set(State::PendingGame, "awaiting game resize callback");
    }
    void adopt(const LiveOwner& o) {
        if (haveOwner_ && sameOwner(o, owner_)) return;
        owner_ = baseline_ = last_ = o;
        inFlight_ = {};
        haveOwner_ = true;
        ++s_.ownerGeneration;
        receipt_ = ssOwned_ = hmdOwned_ = managerOwned_ = confirmed_ = restoring_ = releasing_ = false;
        invalidate();
        start();
    }
    PlannedAction makePlan(const LiveOwner& owner, bool restoring) {
        PlannedAction p{};
        p.valid = true;
        p.restoring = restoring;
        p.releasing = restoring && releasing_;
        p.id = ++nextAction_;
        p.requestSequence = s_.requested.sequence;
        p.ownerGeneration = s_.ownerGeneration;
        p.epoch = s_.epoch;
        p.action.owner = owner;
        return p;
    }
    bool matches(const PlannedAction& p) const {
        return p.valid && inFlight_.valid && p.id == inFlight_.id && p.ownerGeneration == s_.ownerGeneration;
    }
    PlannedAction planRestore(const LiveOwner& owner) {
        auto p = makePlan(owner, true);
        const auto& target = (!releasing_ && confirmed_) ? confirmedOwner_ : baseline_;
        p.action.ss = {
            ssOwned_ && owner.ss == last_.ss, owner.ss, target.ss};
        p.action.contextHmd = {hmdOwned_ && owner.hmd == last_.hmd, owner.hmd, target.hmd};
        p.action.managerHmd = {managerOwned_ && owner.managerHmd == last_.managerHmd, owner.managerHmd, target.managerHmd};
        // A newer manual value wins independently for each field.
        if (!p.action.ss.write) ssOwned_ = false;
        if (!p.action.contextHmd.write) hmdOwned_ = false;
        if (!p.action.managerHmd.write) managerOwned_ = false;
        const float scale = owner.mode == 0 ? target.ss : target.hmd;
        p.outW = output_.width;
        p.outH = output_.height;
        fsr4_quality::scaledDimension(p.outW, scale, p.inW);
        fsr4_quality::scaledDimension(p.outH, scale, p.inH);
        inFlight_ = p;
        return p;
    }
    void fail(const char* reason, bool restore) {
        std::snprintf(failureReason_, sizeof(failureReason_), "%s", reason && reason[0] ? reason : "FSR submission failed");
        failedRequest_ = true;
        invalidate();
        if (restore && receipt_) {
            restoring_ = true;
            set(State::Restoring, failureReason_);
        } else set(State::Refused, failureReason_);
    }
};
    } // namespace renderscale
} // namespace edvr
