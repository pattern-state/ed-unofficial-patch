#include "game_render_scale.h"
#include "game_render_scale_status.h"
#include "../common/code_hook.h"
#include "../common/game_build.h"
#include "../common/log.h"
#include "../common/module_pin.h"
#include "fsr4_engine.h"
#include "game_render_scale_binding.h"
#include "game_render_scale_pins.h"
#include "ui_panel_scale.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
#include <windows.h>
namespace edvr {
namespace {
// Process-retained: an installed relay may still forward through its trampoline.
struct Runtime {
    CodeHook hook;
    renderscale::GameScaleBinding binding;
    char installReason[512]{};
};
std::atomic<Runtime*> runtime{nullptr};
std::mutex installLock, stateLock;
renderscale::ScaleController controller;
std::atomic<bool> stopped{false};
bool featuresAllowed = true;
// Zero is invalid. Valid game HMD values are finite and strictly positive.
std::atomic<uint32_t> observedHmd{0};
renderscale::Snapshot logged{};
bool haveLogged = false;
void publishHmdLocked() {
    const auto s = controller.snapshot();
    uint32_t bits = 0;
    if (featuresAllowed && !stopped.load(std::memory_order_acquire) && s.actualValid &&
        ssaahold::blockModeKnown(s.actual.mode) && ((s.actual.mode != 0) == s.requested.vr) &&
        std::isfinite(s.actual.hmd) && s.actual.hmd>0)
        std::memcpy(&bits, &s.actual.hmd, sizeof(bits));
    observedHmd.store(bits, std::memory_order_release);
}
bool transition(const renderscale::Snapshot& a, const renderscale::Snapshot& b) {
    if (a.state != b.state || a.epoch != b.epoch || a.requested.sequence != b.requested.sequence ||
        a.actualValid != b.actualValid || std::strcmp(a.reason, b.reason) != 0) return true;
    for (unsigned i = 0; i < 3; ++i) {
        const auto& x = a.slots[i];
        const auto& y = b.slots[i];
        if (x.outcome != y.outcome || x.inW != y.inW || x.inH != y.inH || x.outW != y.outW || x.outH != y.outH) return true;
    }
    return false;
}
void logTransition() {
    renderscale::Snapshot s;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        s = controller.snapshot();
        if (haveLogged && !transition(s, logged)) return;
        logged = s;
        haveLogged = true;
    }
    Log::get().note("render quality: request=%llu epoch=%llu profile=%llu owner=%llu scale=%.6g outcome=%s expected=%ux%u->%ux%u actual=%s SS=%.6g HMD=%.6g reason=%s",
        static_cast<unsigned long long>(s.requested.sequence), static_cast<unsigned long long>(s.epoch),
        static_cast<unsigned long long>(s.requested.profileGeneration), static_cast<unsigned long long>(s.ownerGeneration),
        fsr4_quality::scaleFor(s.requested.quality.preset, s.requested.quality.customScale), renderScaleStateName(s.state),
        s.expectedInW, s.expectedInH, s.expectedOutW, s.expectedOutH, s.actualValid?"verified":"unavailable",
        s.actual.ss, s.actual.hmd, s.reason);
}
bool currentOwner(Runtime* value, uintptr_t context, renderscale::LiveOwner* owner) {
    uintptr_t renderer = 0;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        renderer = controller.snapshot().actual.renderer;
    }
    const char* why = nullptr;
    return renderer && renderscale::readLiveOwner(value->binding, renderer, owner, &why) && owner->context == context;
}
void onGameUpdate(uintptr_t renderer) noexcept {
    Runtime* value = runtime.load(std::memory_order_acquire);
    if (!value || stopped.load(std::memory_order_acquire)) return;
    renderscale::LiveOwner owner{};
    const char* why = nullptr;
    if (!renderscale::readLiveOwner(value->binding, renderer, &owner, &why)) {
        {
            std::lock_guard<std::mutex> guard(stateLock);
            controller.ownerUnavailable(why);
            publishHmdLocked();
        }
        return;
    }
    renderscale::PlannedAction plan;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        plan = controller.planGameAction(owner);
        publishHmdLocked();
    }
    if (!plan.valid) {
        return;
    }
    if (plan.restoring && !plan.action.ss.write && !plan.action.contextHmd.write && !plan.action.managerHmd.write) {
        std::lock_guard<std::mutex> guard(stateLock);
        controller.completeUnchangedRelease(plan, owner);
        publishHmdLocked();
        return;
    }
    UiGameResizeReceipt panel{};
    if (!uiPanelScalePrepareGameResize(plan, &panel, &why) || stopped.load(std::memory_order_acquire)) {
        {
            std::lock_guard<std::mutex> guard(stateLock);
            controller.deferGameAction(plan, why?why:"controller shut down");
        }
        uiPanelScaleFinishGameResize(panel, false);
        return;
    }
    const auto result = renderscale::applyScaleAction(value->binding, plan.action);
    {
        std::lock_guard<std::mutex> guard(stateLock);
        controller.recordGameAction(plan, result);
        publishHmdLocked();
    }
    uiPanelScaleFinishGameResize(panel, result.ssWritten || result.hmdWritten || result.managerWritten);
}
} // namespace
bool gameRenderScaleInstall(const char** why) noexcept {
    std::lock_guard<std::mutex> install(installLock);
    if (stopped.load(std::memory_order_acquire)) {
        if (why) *why = "controller shut down";
        return false;
    }
    if (!graphicsModulePinned()) {
        if (why) *why = "graphics module is not pinned";
        return false;
    }
    if (!gameBuildKnown()) {
        if (why) *why = "unsupported game build";
        return false;
    }
    if (auto* existing = runtime.load(std::memory_order_acquire)) {
        if (why) *why = existing->hook.installed()?nullptr:existing->installReason;
        return existing->hook.installed();
    }
    Runtime* value = new (std::nothrow) Runtime();
    if (!value) {
        if (why) *why = "game resize allocation failed";
        return false;
    }
    const char* refusal = nullptr;
    if (!renderscale::resolveBindings({reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr)), renderscale::pins::kImageSize}, &value->binding, &refusal)) {
        // Keep the copied reason in process-retained storage, including failed preparations.
        std::snprintf(value->installReason, sizeof(value->installReason), "%s", refusal?refusal:"binding validation failed");
        runtime.store(value, std::memory_order_release);
        if (why) *why = value->installReason;
        return false;
    }
    runtime.store(value, std::memory_order_release);
    // Callback storage precedes patch publication.
    if (!renderscale::installCheckedHook(value->hook, value->binding, onGameUpdate, graphicsModulePinned(), &refusal)) {
        std::snprintf(value->installReason, sizeof(value->installReason), "%s", refusal?refusal:"game resize hook unavailable");
        if (why) *why = value->installReason;
        return false;
    }
    if (why) *why = nullptr;
    return true;
}
void gameRenderScalePublish(const renderscale::RequestIntent& request) noexcept {
    {
        std::lock_guard<std::mutex> guard(stateLock);
        controller.publishRequest(request);
    }
}
void gameRenderScaleLifecycleTick(bool allowed) noexcept {
    std::lock_guard<std::mutex> guard(stateLock);
    if (stopped.load(std::memory_order_acquire)) return;
    if (featuresAllowed && !allowed) controller.requestRelease();
    featuresAllowed = allowed;
    publishHmdLocked();
}
void gameRenderScaleFrameBoundary(bool eligible, renderscale::OutputBasis output) noexcept {
    if (!eligible || stopped.load(std::memory_order_acquire)) return;
    uint64_t epoch = 0;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        controller.setOutput(output);
        controller.onOwnedBoundary();
        epoch = controller.snapshot().epoch;
    }
    // Never nest the controller state lock with the FSR observation lock or GPU work.
    fsr4SetObservationEpoch(epoch);
    Fsr4DispatchStatus observations[3];
    for (unsigned slot = 0; slot < 3; ++slot) observations[slot] = fsr4DispatchStatus(slot);
    {
        std::lock_guard<std::mutex> guard(stateLock);
        for (unsigned slot = 0; slot < 3; ++slot) controller.observeSubmission(slot, observations[slot]);
    }
    logTransition();
}
void gameRenderScaleAfterSettingsCopy(uintptr_t context) noexcept {
    Runtime* value = runtime.load(std::memory_order_acquire);
    if (!value || stopped.load(std::memory_order_acquire)) return;
    renderscale::LiveOwner owner{};
    if (!currentOwner(value, context, &owner)) return;
    {
        std::lock_guard<std::mutex> guard(stateLock);
        controller.observeGameSettings(owner);
        publishHmdLocked();
    }
}
float gameRenderScaleFlatSetterValue(uintptr_t context, float requested, float held) noexcept {
    Runtime* value = runtime.load(std::memory_order_acquire);
    if (!value || stopped.load(std::memory_order_acquire)) return held;
    renderscale::LiveOwner owner{};
    if (!currentOwner(value, context, &owner)) return held;
    std::lock_guard<std::mutex> guard(stateLock);
    return controller.flatSetterValue(owner, requested, held);
}
void gameRenderScaleProfileChanged() noexcept {
    {
        std::lock_guard<std::mutex> guard(stateLock);
        controller.requestRelease();
        observedHmd.store(0, std::memory_order_release);
    }
}
void gameRenderScaleShutdown() noexcept {
    stopped.store(true, std::memory_order_release);
    renderscale::closeHookAdmission();
    {
        std::lock_guard<std::mutex> guard(stateLock);
        controller.shutdown();
        observedHmd.store(0, std::memory_order_release);
    }
}
renderscale::Snapshot gameRenderScaleSnapshot() noexcept {
    std::lock_guard<std::mutex> guard(stateLock);
    return controller.snapshot();
}
bool gameRenderScaleObservedHmd(float* value) noexcept {
    const uint32_t bits = observedHmd.load(std::memory_order_acquire);
    if (!bits || !value) return false;
    std::memcpy(value, &bits, sizeof(bits));
    return true;
}
std::string gameRenderScaleStatus() {
    return renderScaleStatus(gameRenderScaleSnapshot());
}
} // namespace edvr
