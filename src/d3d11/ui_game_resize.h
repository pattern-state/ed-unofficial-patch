#pragma once
#include "ui_panel_scale.h"
#include "ui_quality_math.h"
#include "ui_sizing_math.h"
#include <cstdint>
namespace edvr {
// Called while the float lock protects generation/admission and factor publication.
inline bool uiGameResizeMayPublish(uint64_t expected, uint64_t current, bool inProgress) noexcept {
    return expected == current && !inProgress;
}
// Called under the float lock before any prepared factor can become visible.
template <class Env> void uiGameResizeInvalidateSetterPublication(UiGameResizeReceipt &receipt, Env &env) {
    receipt.previousReady = env.publicationReady();
    env.setPublicationReady(false);
}
// The same finish section is exercised by the CPU rig and the live float-lock environment.
template <class Env>
bool uiGameResizeFinishSection(const UiGameResizeReceipt &receipt, bool anyGameFieldWritten, Env &env) {
    if (!receipt.active)
        return false;
    UiLockGuard<Env> lock(env);
    if (receipt.generation != env.resizeGeneration() || !env.resizeInProgress())
        return false;
    if (!anyGameFieldWritten)
        env.restorePrevious(receipt);
    env.finishTransition();
    return true;
}
inline bool uiGameResizeFlatPlan(uint32_t intendedW, uint32_t intendedH, uint32_t outputW, uint32_t outputH,
                                 float target, UiPanelPlan *plan) noexcept {
    UiFlatPanelInputs in{};
    in.renderW = intendedW;
    in.renderH = intendedH;
    in.outputW = outputW;
    in.outputH = outputH;
    in.target = target;
    return uiFlatPanelPlanFor(in, plan) == UiFlatPanelRefuse::kNone;
}
inline bool uiGameResizeVrPlan(UiPanelInputs in, uint32_t askedW, float intendedHmd, float intendedSs,
                               UiPanelPlan *plan) noexcept {
    if (!std::isfinite(intendedHmd) || intendedHmd <= 0 || !std::isfinite(intendedSs) || intendedSs <= 0)
        return false;
    in.renderW = uiQualityInternalDim(askedW, intendedHmd);
    in.supersampling = intendedSs;
    return uiPanelPlanFor(in, plan);
}
} // namespace edvr
