// CPU-only checked boundary. External image/object inputs are revalidated; no
// Config, UI, FSR or GPU dependency.
#pragma once
#include <cstddef>
#include <cstdint>
namespace edvr {
class CodeHook;
namespace renderscale {
struct GameImageView {
    uint8_t *base = nullptr;
    size_t size = 0;
};
struct GameScaleBinding {
    GameImageView image{};
    uintptr_t requester = 0, managerVtable = 0;
    bool valid = false;
};
// Identity is the current pointer tuple and copied mode. The controller owns
// its generation number.
struct LiveOwner {
    uintptr_t renderer = 0, context = 0, manager = 0;
    unsigned mode = 0;
    float ss = 0, ssMin = 0, ssMax = 0, hmd = 0, managerHmd = 0;
};
struct FieldAction {
    bool write = false;
    float expected = 0, target = 0;
};
struct ScaleAction {
    LiveOwner owner{};
    FieldAction ss{}, contextHmd{}, managerHmd{};
};
struct ApplyResult {
    bool success = false, ssWritten = false, hmdWritten = false, managerWritten = false, dirtyWritten = false,
         actualValid = false;
    LiveOwner actual{};
    const char *reason = "not attempted";
};
bool resolveBindings(GameImageView, GameScaleBinding *, const char **why) noexcept;
bool readLiveOwner(const GameScaleBinding &, uintptr_t renderer, LiveOwner *, const char **why) noexcept;
ApplyResult applyScaleAction(const GameScaleBinding &, const ScaleAction &) noexcept;
using EntryCallback = void (*)(uintptr_t renderer) noexcept;
// modulePinned is the runtime's actual graphicsModulePinned() receipt, not a
// game-validation bypass.
bool installCheckedHook(CodeHook &, const GameScaleBinding &, EntryCallback, bool modulePinned,
                        const char **why) noexcept;
void closeHookAdmission() noexcept;
} // namespace renderscale
} // namespace edvr
