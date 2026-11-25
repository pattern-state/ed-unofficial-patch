#pragma once
#include "../../src/d3d11/game_render_scale_policy.h"
namespace scale_policy_test {
using namespace edvr;
using namespace edvr::renderscale;
inline LiveOwner owner(bool vr = false) {
    LiveOwner o{};
    o.renderer = 1;
    o.context = 2;
    o.manager = 3;
    o.mode = vr ? 4 : 0;
    o.ss = o.hmd = o.managerHmd = 1;
    o.ssMax = 10;
    return o;
}
inline RequestIntent request(uint64_t sequence = 1, bool vr = false, float scale = .75f) {
    RequestIntent r{};
    r.sequence = sequence;
    r.profileGeneration = 7;
    r.vr = vr;
    r.fsrEnabled = true;
    r.quality.preset = fsr4_quality::Preset::Custom;
    r.quality.customScale = scale;
    return r;
}
inline ApplyResult apply(const PlannedAction& p) {
    ApplyResult r{};
    r.success = true;
    r.actualValid = true;
    r.actual = p.action.owner;
    if (p.action.ss.write) {
        r.actual.ss = p.action.ss.target;
        r.ssWritten = true;
    }
    if (p.action.contextHmd.write) {
        r.actual.hmd = p.action.contextHmd.target;
        r.hmdWritten = true;
    }
    if (p.action.managerHmd.write) {
        r.actual.managerHmd = p.action.managerHmd.target;
        r.managerWritten = true;
    }
    r.dirtyWritten = r.ssWritten || r.hmdWritten || r.managerWritten;
    return r;
}
inline Fsr4DispatchStatus submitted(const Snapshot& s, uint64_t sequence = 1) {
    Fsr4DispatchStatus d{};
    d.epoch = s.epoch;
    d.sequence = sequence;
    d.inW = s.expectedInW;
    d.inH = s.expectedInH;
    d.outW = s.expectedOutW;
    d.outH = s.expectedOutH;
    d.outcome = Fsr4DispatchOutcome::Submitted;
    return d;
}
inline void run() {
    ScaleController c;
    auto o = owner();
    c.publishRequest(request());
    c.setOutput({2000, 1600, true});
    check(c.snapshot().state == State::PendingGame, "request is pending, never applied from saving");
    auto p = c.planGameAction(o);
    check(p.valid && p.action.ss.target == .75f, "flat planned scale");
    c.recordGameAction(p, apply(p));
    o.ss = .75f;
    check(c.snapshot().state == State::PendingBoundary, "write is only rebuild handoff");
    auto old = submitted(c.snapshot());
    c.observeSubmission(0, old);
    check(c.snapshot().state == State::PendingBoundary, "preboundary submission cannot apply");
    c.onOwnedBoundary();
    auto d = submitted(c.snapshot());
    c.observeSubmission(0, d);
    check(c.snapshot().state == State::Applied, "flat exact submitted frame applies");
    c.publishRequest(request(2, false, .5f));
    p = c.planGameAction(o);
    c.recordGameAction(p, apply(p));
    o.ss = .5f;
    c.onOwnedBoundary();
    d = submitted(c.snapshot());
    d.outcome = Fsr4DispatchOutcome::Failed;
    c.observeSubmission(0, d);
    check(c.snapshot().state == State::Restoring, "failed dispatch queues restore");
    p = c.planGameAction(o);
    check(p.valid && p.restoring && p.action.ss.target == .75f, "restore last confirmed scale");
    c.recordGameAction(p, apply(p));
    o.ss = .75f;
    check(c.snapshot().state == State::Refused, "restored request remains refused");
    check(!c.planGameAction(o).valid, "refusal never rebuild storms");
    auto game = request(3);
    game.quality.preset = fsr4_quality::Preset::Game;
    c.publishRequest(game);
    p = c.planGameAction(o);
    check(p.valid && p.action.ss.target == 1, "Game restores original baseline");
    c.recordGameAction(p, apply(p));
    check(c.snapshot().state == State::Game, "Game only complete after callback");

    ScaleController eyes;
    auto v = owner(true);
    eyes.publishRequest(request(1, true));
    eyes.setOutput({2000, 1600, true});
    p = eyes.planGameAction(v);
    eyes.recordGameAction(p, apply(p));
    eyes.onOwnedBoundary();
    d = submitted(eyes.snapshot());
    eyes.observeSubmission(2, d);
    eyes.observeSubmission(1, d);
    check(eyes.snapshot().state == State::AwaitingFsr, "world and right eye cannot replace left eye");
    eyes.observeSubmission(0, d);
    check(eyes.snapshot().state == State::Applied, "both eyes independently submit");
    eyes.publishRequest(request(2, true, .5f));
    v.hmd = v.managerHmd = .75f;
    p = eyes.planGameAction(v);
    eyes.recordGameAction(p, apply(p));
    eyes.onOwnedBoundary();
    eyes.observeSubmission(0, d);
    check(eyes.snapshot().state == State::AwaitingFsr, "old epoch cannot acknowledge resize");
    for (unsigned n = 0; n < 300; ++n) eyes.onOwnedBoundary();
    check(eyes.snapshot().state == State::AwaitingFsr, "paused no dispatch has no wallclock refusal");

    ScaleController timeout;
    timeout.publishRequest(request());
    timeout.setOutput({2000, 1600, true});
    for (unsigned n = 0; n < 119; ++n) timeout.onOwnedBoundary();
    check(timeout.snapshot().state == State::PendingGame, "callback limit not before 120");
    timeout.onOwnedBoundary();
    check(timeout.snapshot().state == State::Refused, "missing callback at 120");
    ScaleController ui;
    ui.publishRequest(request());
    ui.setOutput({2000, 1600, true});
    p = ui.planGameAction(owner());
    ui.deferGameAction(p, "UI geometry unavailable");
    for (unsigned n = 0; n < 200; ++n) ui.onOwnedBoundary();
    check(ui.snapshot().state == State::PendingGame, "observed callback unavailable UI remains pending");
    ScaleController mismatch;
    auto m = owner();
    mismatch.publishRequest(request());
    mismatch.setOutput({2000, 1600, true});
    p = mismatch.planGameAction(m);
    mismatch.recordGameAction(p, apply(p));
    m.ss = .75f;
    mismatch.onOwnedBoundary();
    d = submitted(mismatch.snapshot());
    d.inW = 1;
    for (unsigned n = 1; n <= 119; ++n) {d.sequence = n;
        mismatch.observeSubmission(0, d);
    }
    check(mismatch.snapshot().state == State::AwaitingFsr, "119 distinct wrong sizes are pending");
    for (unsigned n = 0; n < 200; ++n) mismatch.observeSubmission(0, d);
    check(mismatch.snapshot().mismatches[0] == 119, "repeated snapshot consumes once");
    auto pending = d;
    pending.sequence = 120;
    pending.outcome = Fsr4DispatchOutcome::Pending;
    mismatch.observeSubmission(0, pending);
    check(mismatch.snapshot().mismatches[0] == 119, "pending attempts do not consume");
    auto good = submitted(mismatch.snapshot(), 120);
    mismatch.observeSubmission(0, good);
    check(mismatch.snapshot().state == State::Applied && mismatch.snapshot().mismatches[0] == 0, "matching slot resets mismatch counter");
    for (unsigned n = 121; n <= 240; ++n) {d.sequence = n;
        mismatch.observeSubmission(0, d);
    }
    check(mismatch.snapshot().state == State::Restoring, "120 fresh mismatches restore after prior success");
    p = mismatch.planGameAction(m);
    mismatch.recordGameAction(p, apply(p));
    check(!mismatch.planGameAction(m).valid, "mismatch restoration attempted once");

    ScaleController rapid;
    rapid.publishRequest(request());
    rapid.setOutput({2000, 1600, true});
    p = rapid.planGameAction(owner());
    rapid.publishRequest(request(2, false, .5f));
    rapid.recordGameAction(p, apply(p));
    m = owner();
    m.ss = .75f;
    auto latest = rapid.planGameAction(m);
    check(latest.valid && latest.action.ss.target == .5f, "superseded write cannot overwrite newest request");
    rapid.recordGameAction(latest, apply(latest));
    m.ss = .5f;
    rapid.onOwnedBoundary();
    d = submitted(rapid.snapshot());
    rapid.publishRequest(request(3, false, .75f));
    p = rapid.planGameAction(m);
    rapid.recordGameAction(p, apply(p));
    rapid.onOwnedBoundary();
    rapid.observeSubmission(0, d);
    check(rapid.snapshot().state == State::AwaitingFsr, "A B A rejects B epoch");
    d = submitted(rapid.snapshot());
    rapid.observeSubmission(0, d);
    check(rapid.snapshot().state == State::Applied, "A B A accepts new A epoch");
    auto invalid = request(4);
    invalid.quality.valid = false;
    rapid.publishRequest(invalid);
    m.ss = .75f;
    check(!rapid.planGameAction(m).valid && rapid.snapshot().actual.ss == .75f, "invalid config retains actual and writes nothing");
    check(rapid.snapshot().state == State::Refused, "invalid config invalidates applied state");
    game = request(5);
    game.quality.preset = fsr4_quality::Preset::Game;
    rapid.publishRequest(game);
    m.ss = .9f;
    p = rapid.planGameAction(m);
    check(p.valid && !p.action.ss.write, "manual selection wins release");
    rapid.recordGameAction(p, apply(p));
    check(rapid.snapshot().state == State::Game, "manual-preserving release completes");

    ScaleController replaced;
    replaced.publishRequest(request());
    replaced.setOutput({2000, 1600, true});
    p = replaced.planGameAction(owner());
    replaced.recordGameAction(p, apply(p));
    game = request(2);
    game.quality.preset = fsr4_quality::Preset::Game;
    replaced.publishRequest(game);
    m = owner();
    m.context = 22;
    check(!replaced.planGameAction(m).valid, "new owner discards outgoing receipt without old-owner write");
    ScaleController partial;
    partial.publishRequest(request(1, true));
    partial.setOutput({2000, 1600, true});
    p = partial.planGameAction(owner(true));
    auto result = apply(p);
    result.success = false;
    result.managerWritten = false;
    result.actual.managerHmd = 1;
    result.reason = "manager setter failed";
    partial.recordGameAction(p, result);
    m = owner(true);
    m.hmd = .75f;
    p = partial.planGameAction(m);
    check(p.restoring && p.action.contextHmd.write && !p.action.managerHmd.write, "partial write restores only owned context field");
    partial.recordGameAction(p, apply(p));
    check(partial.snapshot().state == State::Refused, "partial restore keeps selected request refused");
    ScaleController settings;
    settings.publishRequest(request());
    settings.setOutput({2000, 1600, true});
    p = settings.planGameAction(owner());
    settings.recordGameAction(p, apply(p));
    m = owner();
    m.ss = .85f;
    settings.observeGameSettings(m);
    game = request(2);
    game.quality.preset = fsr4_quality::Preset::Game;
    settings.publishRequest(game);
    p = settings.planGameAction(m);
    check(!p.action.ss.write, "fresh settings copy preserves genuine baseline");
    ScaleController bounds;
    bounds.publishRequest(request());
    bounds.setOutput({2000, 1600, true});
    m = owner();
    m.ssMin = .8f;
    check(!bounds.planGameAction(m).valid && bounds.snapshot().state == State::Refused, "unsupported game bounds preflight refuses");
    bounds.shutdown();
    bounds.publishRequest(request(2));
    check(!bounds.planGameAction(owner()).valid, "shutdown closes action admission");
    ScaleController releaseInFlight;
    releaseInFlight.publishRequest(request());
    releaseInFlight.setOutput({2000,1600,true});
    p = releaseInFlight.planGameAction(owner());
    game = request(2);
    game.quality.preset = fsr4_quality::Preset::Game;
    releaseInFlight.publishRequest(game);
    releaseInFlight.recordGameAction(p,apply(p));
    m = owner();
    m.ss = .75f;
    p = releaseInFlight.planGameAction(m);
    check(p.restoring && p.action.ss.target == 1,"Game racing prior write still releases it");
    releaseInFlight.recordGameAction(p,apply(p));
    check(releaseInFlight.snapshot().state == State::Game,"racing release completed after callback");

    ScaleController setter;
    setter.publishRequest(request());
    setter.setOutput({2000,1600,true});
    p = setter.planGameAction(owner());
    setter.recordGameAction(p,apply(p));
    m = owner();
    m.ss = .75f;
    check(setter.flatSetterValue(m,.9f,.9f) == .75f,"verified managed flat setter keeps effective scale");
    setter.publishRequest(game);
    p = setter.planGameAction(m);
    check(p.action.ss.target == .9f,"flat setter retains genuine requested baseline for Game");
    setter.recordGameAction(p,apply(p));

    ScaleController unchanged;
    unchanged.publishRequest(request());
    unchanged.setOutput({2000,1600,true});
    p = unchanged.planGameAction(owner());
    unchanged.recordGameAction(p,apply(p));
    unchanged.publishRequest(game);
    m = owner();
    m.ss = .8f;
    p = unchanged.planGameAction(m);
    check(unchanged.completeUnchangedRelease(p,m),"fresh validated genuine value completes no-write release");
    check(unchanged.snapshot().state == State::Game && unchanged.snapshot().actual.ss == .8f,
              "no-write release records retained game value without resize");

    ScaleController vrMode;
    vrMode.publishRequest(request(1,true));
    vrMode.setOutput({2000,1600,true});
    m = owner(true);
    m.mode = 6;
    check(vrMode.planGameAction(m).action.contextHmd.write,"all known nonzero copied modes share VR policy");

    ScaleController outputRace;
    outputRace.publishRequest(request());
    outputRace.setOutput({2000,1600,true});
    p = outputRace.planGameAction(owner());
    outputRace.setOutput({2400,1800,true});
    outputRace.recordGameAction(p,apply(p));
    check(outputRace.snapshot().state == State::PendingGame,"output change makes in-flight result historical");
    m = owner();
    m.ss = .75f;
    p = outputRace.planGameAction(m);
    check(p.outW == 2400 && p.inW == 1800,"output race retries against newest valid basis");
    outputRace.recordGameAction(p,apply(p));
    outputRace.onOwnedBoundary();
    d = submitted(outputRace.snapshot());
    outputRace.observeSubmission(0,d);
    const auto noOpEpoch = outputRace.snapshot().epoch;
    outputRace.publishRequest(request(2));
    check(outputRace.snapshot().state == State::Applied && outputRace.snapshot().epoch == noOpEpoch,
          "identical repeated choice does not rebuild or discard established acceptance");
    game = request(3);
    game.quality.preset = fsr4_quality::Preset::Game;
    outputRace.publishRequest(game);
    p = outputRace.planGameAction(m);
    outputRace.publishRequest(request(4,false,.5f));
    outputRace.recordGameAction(p,apply(p));
    m.ss = 1;
    p = outputRace.planGameAction(m);
    check(p.valid && !p.restoring && p.action.ss.target == .5f,"new request racing release waits for its completion once");

    ScaleController wrongSizeFailure;
    wrongSizeFailure.publishRequest(request());
    wrongSizeFailure.setOutput({2000, 1600, true});
    p = wrongSizeFailure.planGameAction(owner());
    wrongSizeFailure.recordGameAction(p, apply(p));
    wrongSizeFailure.onOwnedBoundary();
    d = submitted(wrongSizeFailure.snapshot());
    d.outcome = Fsr4DispatchOutcome::Failed;
    d.inW = 1;
    wrongSizeFailure.observeSubmission(0, d);
    check(wrongSizeFailure.snapshot().state == State::AwaitingFsr &&
          wrongSizeFailure.snapshot().mismatches[0] == 1,
          "wrong-size terminal failure counts mismatch instead of immediate restoration");
    d = submitted(wrongSizeFailure.snapshot(), 2);
    d.outcome = Fsr4DispatchOutcome::Failed;
    wrongSizeFailure.observeSubmission(0, d);
    check(wrongSizeFailure.snapshot().state == State::Restoring,
          "failed matching-size attempt immediately queues restoration");

    ScaleController profileRelease;
    profileRelease.publishRequest(request());
    profileRelease.setOutput({2000, 1600, true});
    p = profileRelease.planGameAction(owner());
    profileRelease.requestRelease();
    profileRelease.recordGameAction(p, apply(p));
    m = owner();
    m.ss = .75f;
    p = profileRelease.planGameAction(m);
    check(p.valid && p.restoring && p.action.ss.target == 1,
          "profile release before first write receipt still restores without later publication");
    profileRelease.recordGameAction(p, apply(p));
    check(profileRelease.snapshot().state == State::Game,
          "explicit profile release stays released without later publication");
    ScaleController profileNext;
    profileNext.publishRequest(request());
    profileNext.setOutput({2000, 1600, true});
    p = profileNext.planGameAction(owner());
    profileNext.requestRelease();
    auto nextProfile = request(2, false, .5f);
    nextProfile.profileGeneration = 8;
    profileNext.publishRequest(nextProfile);
    profileNext.recordGameAction(p, apply(p));
    p = profileNext.planGameAction(m);
    check(p.restoring && p.action.ss.target == 1,
          "new profile publication waits for outgoing first-write release");
    profileNext.recordGameAction(p, apply(p));
    m.ss = 1;
    p = profileNext.planGameAction(m);
    check(p.valid && !p.restoring && p.action.ss.target == .5f,
          "new profile managed scale follows completed outgoing release");

    ScaleController steady;
    steady.publishRequest(request());
    steady.setOutput({2000, 1600, true});
    p = steady.planGameAction(owner());
    steady.recordGameAction(p, apply(p));
    steady.onOwnedBoundary();
    d = submitted(steady.snapshot());
    steady.observeSubmission(0, d);
    const auto unchangedRevision = steady.snapshot().revision;
    d.sequence = 2;
    steady.observeSubmission(0, d);
    check(steady.snapshot().revision == unchangedRevision && steady.snapshot().slots[0].sequence == 2,
          "identical submitted frames retain latest sequence without reraster revision");

    m = owner();
    m.ss = .75f;
    steady.planGameAction(m);
    check(steady.snapshot().revision == unchangedRevision,
          "unchanged validated game callback does not reraster accepted status");
    steady.observeGameSettings(m);
    check(steady.snapshot().revision == unchangedRevision,
          "unchanged genuine settings copy does not reraster accepted status");
    m.hmd = 1.1f;
    steady.planGameAction(m);
    check(steady.snapshot().revision > unchangedRevision,
          "changed validated actual game value refreshes copied UI status");

    ScaleController replaceDuringWrite;
    replaceDuringWrite.publishRequest(request());
    replaceDuringWrite.setOutput({2000, 1600, true});
    p = replaceDuringWrite.planGameAction(owner());
    m = owner();
    m.context = 22;
    replaceDuringWrite.observeGameSettings(m);
    replaceDuringWrite.recordGameAction(p, apply(p));
    check(replaceDuringWrite.planGameAction(m).valid,
          "new owner during unlocked write discards old action admission token");
    ScaleController releaseDuringRollback;
    m = owner();
    releaseDuringRollback.publishRequest(request());
    releaseDuringRollback.setOutput({2000, 1600, true});
    p = releaseDuringRollback.planGameAction(m);
    releaseDuringRollback.recordGameAction(p, apply(p));
    m.ss = .75f;
    releaseDuringRollback.onOwnedBoundary();
    releaseDuringRollback.observeSubmission(0, submitted(releaseDuringRollback.snapshot()));
    releaseDuringRollback.publishRequest(request(2, false, .5f));
    p = releaseDuringRollback.planGameAction(m);
    releaseDuringRollback.recordGameAction(p, apply(p));
    m.ss = .5f;
    releaseDuringRollback.onOwnedBoundary();
    d = submitted(releaseDuringRollback.snapshot());
    d.outcome = Fsr4DispatchOutcome::Failed;
    releaseDuringRollback.observeSubmission(0, d);
    p = releaseDuringRollback.planGameAction(m);
    releaseDuringRollback.requestRelease();
    releaseDuringRollback.recordGameAction(p, apply(p));
    m.ss = .75f;
    p = releaseDuringRollback.planGameAction(m);
    check(p.valid && p.restoring && p.releasing && p.action.ss.target == 1,
          "explicit release racing rollback still restores original baseline");
    releaseDuringRollback.recordGameAction(p, apply(p));
    check(releaseDuringRollback.snapshot().state == State::Game,
          "explicit release racing rollback completes once original baseline restored");
    ScaleController invalidRelease;
    invalidRelease.publishRequest(request());
    invalidRelease.setOutput({2000, 1600, true});
    p = invalidRelease.planGameAction(owner());
    invalidRelease.recordGameAction(p, apply(p));
    invalid = request(2);
    invalid.quality.valid = false;
    invalidRelease.publishRequest(invalid);
    invalidRelease.requestRelease();
    m = owner();
    m.ss = .75f;
    p = invalidRelease.planGameAction(m);
    check(p.valid && p.restoring && p.action.ss.target == 1,
          "explicit profile release restores retained receipt even with invalid request");
    invalidRelease.recordGameAction(p, apply(p));
    check(invalidRelease.snapshot().state == State::Game, "invalid outgoing profile releases to Game");

    ScaleController invalidAaOff;
    invalidAaOff.publishRequest(request());
    invalidAaOff.setOutput({2000, 1600, true});
    p = invalidAaOff.planGameAction(owner());
    invalidAaOff.recordGameAction(p, apply(p));
    invalid = request(2);
    invalid.quality.valid = false;
    invalid.fsrEnabled = false;
    invalidAaOff.publishRequest(invalid);
    m = owner();
    m.ss = .75f;
    p = invalidAaOff.planGameAction(m);
    check(invalidAaOff.snapshot().state == State::Restoring && p.restoring && p.action.ss.target == 1,
          "explicit AA-off releases retained receipt despite invalid quality configuration");

}
} // namespace scale_policy_test
