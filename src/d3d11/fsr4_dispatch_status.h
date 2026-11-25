// CPU-only submission observations. The engine supplies synchronization; this
// reducer is also driven directly by the GPU-free engine rig.
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

namespace edvr {
enum class Fsr4DispatchOutcome { Pending, Submitted, Failed };
struct Fsr4SubmittedDispatch {
    bool valid = false;
    uint64_t epoch = 0, sequence = 0;
    uint32_t inW = 0, inH = 0, outW = 0, outH = 0;
};
struct Fsr4DispatchStatus {
    uint64_t epoch = 0, sequence = 0;
    uint32_t inW = 0, inH = 0, outW = 0, outH = 0;
    Fsr4DispatchOutcome outcome = Fsr4DispatchOutcome::Pending;
    char reason[512] = {};
    // Historical submission only; current acceptance requires outcome Submitted.
    Fsr4SubmittedDispatch lastSubmitted;
};
inline std::string fsr4DispatchStatusText(std::string line, const Fsr4DispatchStatus* slots, bool flat) {
    for (unsigned slot = 0; slot < (flat ? 1u : 3u); ++slot) {
        const auto& s = slots[slot];
        char text[256];
        const char* outcome = s.outcome == Fsr4DispatchOutcome::Submitted ? "submitted" :
                              s.outcome == Fsr4DispatchOutcome::Failed ? "failed" : "pending";
        if (flat) std::snprintf(text, sizeof(text), "; Flat %s %ux%u -> %ux%u", outcome, s.inW, s.inH, s.outW, s.outH);
        else std::snprintf(text, sizeof(text), "; slot %u%s %s %ux%u -> %ux%u", slot,
                      slot == 2 ? " (VR screen/world)" : "", outcome, s.inW, s.inH, s.outW, s.outH);
        line += text;
        if (s.reason[0]) { line += ": "; line += s.reason; }
        if (s.outcome != Fsr4DispatchOutcome::Submitted && s.lastSubmitted.valid) {
            const auto& h = s.lastSubmitted;
            std::snprintf(text, sizeof(text), " (historical last submission %ux%u -> %ux%u, epoch %llu)",
                          h.inW, h.inH, h.outW, h.outH, static_cast<unsigned long long>(h.epoch));
            line += text;
        }
    }
    return line;
}
struct Fsr4DispatchAttempt {
    bool valid = false;
    unsigned slot = 0;
    uint64_t epoch = 0, sequence = 0;
};
class Fsr4DispatchObservations {
public:
    static constexpr unsigned slotCount = 3;
    void setEpoch(uint64_t epoch) {
        if (epoch == epoch_) return;
        epoch_ = epoch;
        invalidate("awaiting submission for current request");
    }
    void invalidate(const char* reason) {
        for (auto& s : slots_) {
            const auto historical = s.lastSubmitted;
            s = Fsr4DispatchStatus{};
            s.epoch = epoch_;
            s.sequence = ++sequence_;
            s.lastSubmitted = historical;
            std::snprintf(s.reason, sizeof(s.reason), "%s", reason);
        }
    }
    Fsr4DispatchAttempt begin(unsigned slot, uint32_t w, uint32_t h, uint32_t outW, uint32_t outH) {
        if (slot >= slotCount) return {};
        auto& s = slots_[slot];
        s.epoch = epoch_;
        s.sequence = ++sequence_;
        s.inW = w; s.inH = h; s.outW = outW; s.outH = outH;
        s.outcome = Fsr4DispatchOutcome::Pending;
        s.reason[0] = '\0';
        return {true, slot, s.epoch, s.sequence};
    }
    void finish(const Fsr4DispatchAttempt& attempt, bool submitted, const char* reason) {
        if (!attempt.valid || attempt.slot >= slotCount) return;
        auto& s = slots_[attempt.slot];
        if (s.epoch != attempt.epoch || s.sequence != attempt.sequence) return;
        s.outcome = submitted ? Fsr4DispatchOutcome::Submitted : Fsr4DispatchOutcome::Failed;
        std::snprintf(s.reason, sizeof(s.reason), "%s", submitted ? "" :
                      (reason && reason[0] ? reason : "FSR submission refused without a reason"));
        if (submitted) s.lastSubmitted = {true, s.epoch, s.sequence, s.inW, s.inH, s.outW, s.outH};
    }
    Fsr4DispatchStatus snapshot(unsigned slot) const {
        return slot < slotCount ? slots_[slot] : Fsr4DispatchStatus{};
    }
private:
    uint64_t epoch_ = 0, sequence_ = 0;
    Fsr4DispatchStatus slots_[slotCount];
};
} // namespace edvr
