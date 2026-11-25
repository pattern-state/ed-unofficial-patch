#pragma once
#include "game_render_scale_policy.h"
#include <cstdint>
#include <string>
namespace edvr {
bool gameRenderScaleInstall(const char **why = nullptr) noexcept;
// Immutable settings-owner publication. The publisher supplies monotonic request/profile IDs.
void gameRenderScalePublish(const renderscale::RequestIntent&) noexcept;
void gameRenderScaleLifecycleTick(bool featuresAllowed) noexcept;
// D3D owner only. eligible means S_OK/non-TEST/canonical device and first-Present thread.
void gameRenderScaleFrameBoundary(bool eligible, renderscale::OutputBasis) noexcept;
void gameRenderScaleAfterSettingsCopy(uintptr_t context) noexcept;
float gameRenderScaleFlatSetterValue(uintptr_t context, float requested, float held) noexcept;
void gameRenderScaleProfileChanged() noexcept;
void gameRenderScaleShutdown() noexcept;
renderscale::Snapshot gameRenderScaleSnapshot() noexcept;
std::string gameRenderScaleStatus();
// Atomic validated live HMD value; false means consumers use their file-cache fallback.
bool gameRenderScaleObservedHmd(float* value) noexcept;
} // namespace edvr
