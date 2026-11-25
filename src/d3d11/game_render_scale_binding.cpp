#include "game_render_scale_binding.h"
#include "../common/code_hook.h"
#include "game_render_scale_pins.h"
#include "vr_ssaa_hold_math.h" // owns the known copied stereoscopic-mode range
#include <atomic>
#include <cmath>
#include <cstring>
#include <windows.h>

extern "C" {
void gameRenderScaleEntry();
__declspec(align(8)) void *g_gameRenderScaleOriginal = nullptr;
}
namespace edvr {
namespace renderscale {
namespace {
std::atomic<EntryCallback> callback{nullptr};
std::atomic<bool> admission{false};
thread_local bool inside = false;
bool refuse(const char **why, const char *text) noexcept {
    if (why)
        *why = text;
    return false;
}
bool copy(uintptr_t p, void *out, size_t n) noexcept {
    if (!p || !out)
        return false;
    __try {
        std::memcpy(out, reinterpret_cast<void *>(p), n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
template <class T> bool read(uintptr_t p, T *out) noexcept { return copy(p, out, sizeof(T)); }
bool store(uintptr_t p, float value) noexcept {
    __try {
        *reinterpret_cast<volatile float *>(p) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
bool dirty(uintptr_t p) noexcept {
    __try {
        *reinterpret_cast<volatile uint8_t *>(p) = 1;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
bool imageCopy(GameImageView v, size_t off, void *out, size_t n) noexcept {
    return v.base && off <= v.size && n <= v.size - off && copy(uintptr_t(v.base) + off, out, n);
}
bool positive(float v) noexcept { return std::isfinite(v) && v > 0; }
bool sameOwner(const LiveOwner &a, const LiveOwner &b) noexcept {
    return a.renderer == b.renderer && a.context == b.context && a.manager == b.manager && a.mode == b.mode;
}
bool callSetter(uintptr_t at, uintptr_t manager, float value) noexcept {
    __try {
        reinterpret_cast<void (*)(void *, float)>(at)(reinterpret_cast<void *>(manager), value);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
// Same register-free absolute relay shape as the existing VR window
// trampolines.
constexpr size_t kRelayBytes = 14;
uint8_t *allocateRelayNear(uintptr_t target) {
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    const uintptr_t granularity = info.dwAllocationGranularity;
    const uintptr_t floor = reinterpret_cast<uintptr_t>(info.lpMinimumApplicationAddress);
    const uintptr_t ceiling = reinterpret_cast<uintptr_t>(info.lpMaximumApplicationAddress);
    const uintptr_t distance = uintptr_t(INT32_MAX) - 0x10000u;
    uintptr_t at = target > distance ? target - distance : floor;
    if (at < floor)
        at = floor;
    const uintptr_t limit = target > ceiling - distance ? ceiling : target + distance;
    while (at < limit) {
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(reinterpret_cast<void *>(at), &region, sizeof(region)))
            break;
        const uintptr_t start = reinterpret_cast<uintptr_t>(region.BaseAddress);
        if (region.RegionSize > UINTPTR_MAX - start)
            break;
        const uintptr_t end = start + region.RegionSize;
        if (region.State == MEM_FREE) {
            uintptr_t candidate = at > start ? at : start;
            if (candidate > UINTPTR_MAX - (granularity - 1))
                break;
            candidate = (candidate + granularity - 1) & ~(granularity - 1);
            if (candidate < limit && candidate < end && end - candidate >= 4096) {
                auto *p = static_cast<uint8_t *>(
                    VirtualAlloc(reinterpret_cast<void *>(candidate), 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
                if (p)
                    return p;
            }
        }
        if (end <= at)
            break;
        at = end;
    }
    return nullptr;
}

uint8_t *buildRelayNear(uintptr_t target, void *replacement) {
    uint8_t *relay = allocateRelayNear(target);
    if (!relay)
        return nullptr;
    const uint8_t body[kRelayBytes] = {0xFF, 0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::memcpy(relay, body, sizeof(body));
    const uintptr_t to = reinterpret_cast<uintptr_t>(replacement);
    std::memcpy(relay + 6, &to, 8);
    DWORD old = 0;
    if (!VirtualProtect(relay, 4096, PAGE_EXECUTE_READ, &old) ||
        !FlushInstructionCache(GetCurrentProcess(), relay, kRelayBytes)) {
        VirtualFree(relay, 0, MEM_RELEASE);
        return nullptr;
    }
    return relay; // kept for the process lifetime: the installed patch jumps here
}

bool prepareOriginal(void *trampoline, void *) noexcept {
    InterlockedExchangePointer(&g_gameRenderScaleOriginal, trampoline);
    return trampoline != nullptr;
}
} // namespace
bool resolveBindings(GameImageView image, GameScaleBinding *out, const char **why) noexcept {
    if (!out)
        return refuse(why, "missing binding output");
    *out = {};
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS64 nt{};
    if (!imageCopy(image, 0, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0 ||
        !imageCopy(image, size_t(dos.e_lfanew), &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return refuse(why, "unreadable game PE identity");
    if (nt.FileHeader.TimeDateStamp != pins::kTimestamp || nt.OptionalHeader.SizeOfImage != pins::kImageSize ||
        image.size != pins::kImageSize)
        return refuse(why, "unsupported game build");
    uint8_t bytes[128]{};
    for (const auto &w : pins::kWitnesses)
        if (w.size > sizeof(bytes) || !imageCopy(image, w.rva, bytes, w.size) || std::memcmp(bytes, w.bytes, w.size))
            return refuse(why, "game instruction witness mismatch");
    uintptr_t pointer = 0;
    if (!imageCopy(image, pins::kRegistrationTable, &pointer, sizeof(pointer)) ||
        pointer != uintptr_t(image.base) + pins::kWrapper)
        return refuse(why, "game requester registration mismatch");
    if (!imageCopy(image, pins::kManagerVtable + 0x100, &pointer, sizeof(pointer)) ||
        pointer != uintptr_t(image.base) + pins::kHmdSetter ||
        !imageCopy(image, pins::kManagerVtable + 0x108, &pointer, sizeof(pointer)) ||
        pointer != uintptr_t(image.base) + pins::kHmdGetter)
        return refuse(why, "game HMD vtable witness mismatch");
    out->image = image;
    out->requester = uintptr_t(image.base) + pins::kRequester;
    out->managerVtable = uintptr_t(image.base) + pins::kManagerVtable;
    out->valid = true;
    if (why)
        *why = nullptr;
    return true;
}
bool readLiveOwner(const GameScaleBinding &b, uintptr_t renderer, LiveOwner *out, const char **why) noexcept {
    if (!out)
        return refuse(why, "missing owner output");
    *out = {};
    LiveOwner o{};
    o.renderer = renderer;
    if (!b.valid || !renderer || !read(renderer + pins::kRendererContext, &o.context) || !o.context ||
        !read(o.context + pins::kMode, &o.mode) || !ssaahold::blockModeKnown(o.mode))
        return refuse(why, "unreadable renderer or unknown copied mode");
    if (!read(o.context + pins::kSs, &o.ss) || !read(o.context + pins::kSsMin, &o.ssMin) ||
        !read(o.context + pins::kSsMax, &o.ssMax) || !std::isfinite(o.ss) || !std::isfinite(o.ssMin) ||
        !std::isfinite(o.ssMax) || o.ssMin < 0 || o.ssMin > o.ssMax || o.ss < o.ssMin || o.ss > o.ssMax)
        return refuse(why, "invalid live supersampling bounds");
    if (!read(o.context + pins::kHmd, &o.hmd) || !positive(o.hmd))
        return refuse(why, "invalid live HMD scale");
    if (o.mode != 0) {
        uintptr_t vt = 0, settings = 0, alias = 0, setter = 0, getter = 0;
        if (!read(renderer + pins::kRendererManager, &o.manager) || !o.manager || !read(o.manager, &vt) ||
            vt != b.managerVtable || !read(vt + 0x100, &setter) ||
            setter != uintptr_t(b.image.base) + pins::kHmdSetter || !read(vt + 0x108, &getter) ||
            getter != uintptr_t(b.image.base) + pins::kHmdGetter ||
            !read(renderer + pins::kRendererSettings, &settings) || !settings ||
            !read(settings + pins::kSettingsManager, &alias) || alias != o.manager ||
            !read(o.manager + pins::kManagerHmd, &o.managerHmd) || !positive(o.managerHmd))
            return refuse(why, "HMD manager ownership witness mismatch");
    }
    *out = o;
    if (why)
        *why = nullptr;
    return true;
}
ApplyResult applyScaleAction(const GameScaleBinding &b, const ScaleAction &a) noexcept {
    ApplyResult r{};
    LiveOwner o{};
    const char *why = nullptr;
    if (!readLiveOwner(b, a.owner.renderer, &o, &why)) {
        r.reason = why;
        return r;
    }
    if (!sameOwner(o, a.owner)) {
        r.reason = "game owner changed before write";
        return r;
    }
    if (!a.ss.write && !a.contextHmd.write && !a.managerHmd.write) {
        r.reason = "empty game action";
        return r;
    }
    if ((a.ss.write && (o.mode != 0 || !std::isfinite(a.ss.target) || a.ss.target < o.ssMin || a.ss.target > o.ssMax ||
                        o.ss != a.ss.expected)) ||
        (a.contextHmd.write && (o.mode == 0 || !positive(a.contextHmd.target) || o.hmd != a.contextHmd.expected)) ||
        (a.managerHmd.write &&
         (o.mode == 0 || !positive(a.managerHmd.target) || o.managerHmd != a.managerHmd.expected))) {
        r.reason = "game scale preflight refused";
        return r;
    }
    bool ok = true;
    if (a.ss.write) {
        r.ssWritten = store(o.context + pins::kSs, a.ss.target);
        ok = r.ssWritten;
    }
    if (ok && a.contextHmd.write) {
        r.hmdWritten = store(o.context + pins::kHmd, a.contextHmd.target);
        ok = r.hmdWritten;
    }
    if (ok && a.managerHmd.write) {
        r.managerWritten = callSetter(uintptr_t(b.image.base) + pins::kHmdSetter, o.manager, a.managerHmd.target);
        ok = r.managerWritten;
    }
    if (r.ssWritten || r.hmdWritten || r.managerWritten) {
        r.dirtyWritten = dirty(o.renderer + pins::kDirty);
        ok = ok && r.dirtyWritten;
    }
    r.actualValid = readLiveOwner(b, o.renderer, &r.actual, &why) && sameOwner(o, r.actual);
    r.success = ok && r.actualValid && (!a.ss.write || r.actual.ss == a.ss.target) &&
                (!a.contextHmd.write || r.actual.hmd == a.contextHmd.target) &&
                (!a.managerHmd.write || r.actual.managerHmd == a.managerHmd.target);
    r.reason = r.success ? nullptr : "game scale action partially failed; checked restoration required";
    return r;
}
bool installCheckedHook(CodeHook &hook, const GameScaleBinding &b, EntryCallback observed, bool modulePinned,
                        const char **why) noexcept {
    if (!modulePinned)
        return refuse(why, "graphics module is not pinned");
    if (!b.valid || !observed || hook.installed())
        return refuse(why, "invalid or already installed game resize hook");
    uint8_t *relay = buildRelayNear(b.requester, reinterpret_cast<void *>(&gameRenderScaleEntry));
    if (!relay)
        return refuse(why, "no executable relay within reach of game requester");
    callback.store(observed, std::memory_order_release);
    admission.store(true, std::memory_order_release);
    void *original = nullptr;
    if (!hook.install(reinterpret_cast<void *>(b.requester), relay, &original, "game render scale", prepareOriginal,
                      nullptr)) {
        admission.store(false, std::memory_order_release);
        callback.store(nullptr, std::memory_order_release);
        InterlockedExchangePointer(&g_gameRenderScaleOriginal, nullptr);
        VirtualFree(relay, 0, MEM_RELEASE);
        return refuse(why, hook.refusal());
    }
    if (why)
        *why = nullptr;
    // The installed patch owns this relay for the remainder of the process
    // lifetime.
    return true;
}
void closeHookAdmission() noexcept { admission.store(false, std::memory_order_release); }
extern "C" void gameRenderScaleEntryObserved(uintptr_t renderer) noexcept {
    if (inside || !admission.load(std::memory_order_acquire))
        return;
    inside = true;
    auto fn = callback.load(std::memory_order_acquire);
    if (fn && admission.load(std::memory_order_acquire))
        fn(renderer);
    inside = false;
}
} // namespace renderscale
} // namespace edvr
