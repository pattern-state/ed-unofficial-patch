// Synthetic external game objects exercise the real binding, entry and
// CodeHook.
#include "../../src/common/code_hook.h"
#include "../../src/common/log.h"
#include "../../src/d3d11/game_render_scale_binding.h"
#include "../../src/d3d11/game_render_scale_pins.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>
#include <windows.h>
namespace edvr {
Log &Log::get() {
    static Log value;
    return value;
}
Log::~Log() {}
void Log::note(const char *format, ...) {
    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    std::putchar('\n');
    va_end(args);
}
int guardFilter(unsigned long, const char *) { return EXCEPTION_EXECUTE_HANDLER; }
} // namespace edvr
using namespace edvr::renderscale;
unsigned checks = 0, failures = 0;
void check(bool v, const char *why) {
    ++checks;
    if (!v) {
        ++failures;
        std::printf("FAIL %s\n", why);
    }
}
// The same pure reducer cases also run in the GPU-free isolated native check.
#include "game_render_scale_policy_test.h"
template <class T> void put(uintptr_t p, size_t off, T v) {
    std::memcpy(reinterpret_cast<void *>(p + off), &v, sizeof(v));
}
struct Fixture {
    uint8_t *image;
    std::vector<uint8_t> renderer, context, manager, settings;
    GameScaleBinding binding{};
    Fixture()
        : image(static_cast<uint8_t *>(
              VirtualAlloc(nullptr, pins::kImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE))),
          renderer(0x1200), context(0x3800), manager(0x500), settings(0x800) {
        check(image != nullptr, "allocate external image");
        if (!image)
            return;
        IMAGE_DOS_HEADER dos{};
        dos.e_magic = IMAGE_DOS_SIGNATURE;
        dos.e_lfanew = 0x80;
        std::memcpy(image, &dos, sizeof(dos));
        IMAGE_NT_HEADERS64 nt{};
        nt.Signature = IMAGE_NT_SIGNATURE;
        nt.FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
        nt.FileHeader.TimeDateStamp = pins::kTimestamp;
        nt.OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
        nt.OptionalHeader.SizeOfImage = pins::kImageSize;
        std::memcpy(image + 0x80, &nt, sizeof(nt));
        for (const auto &w : pins::kWitnesses)
            std::memcpy(image + w.rva, w.bytes, w.size);
        put(uintptr_t(image), pins::kRegistrationTable, uintptr_t(image) + pins::kWrapper);
        put(uintptr_t(image), pins::kManagerVtable + 0x100, uintptr_t(image) + pins::kHmdSetter);
        put(uintptr_t(image), pins::kManagerVtable + 0x108, uintptr_t(image) + pins::kHmdGetter);
        put(uintptr_t(renderer.data()), pins::kRendererContext, uintptr_t(context.data()));
        put(uintptr_t(renderer.data()), pins::kRendererManager, uintptr_t(manager.data()));
        put(uintptr_t(renderer.data()), pins::kRendererSettings, uintptr_t(settings.data()));
        put(uintptr_t(settings.data()), pins::kSettingsManager, uintptr_t(manager.data()));
        put(uintptr_t(manager.data()), 0, uintptr_t(image) + pins::kManagerVtable);
        put(uintptr_t(context.data()), pins::kSs, 1.f);
        put(uintptr_t(context.data()), pins::kSsMin, 0.f);
        put(uintptr_t(context.data()), pins::kSsMax, std::numeric_limits<float>::max());
        put(uintptr_t(context.data()), pins::kHmd, 1.f);
        put(uintptr_t(manager.data()), pins::kManagerHmd, 1.f);
    }
    ~Fixture() {
        if (image)
            VirtualFree(image, 0, MEM_RELEASE);
    }
    bool resolve(const char **why) { return resolveBindings({image, pins::kImageSize}, &binding, why); }
    bool owner(LiveOwner *o, const char **why) { return readLiveOwner(binding, uintptr_t(renderer.data()), o, why); }
};
void bindingTests() {
    Fixture f;
    const char *why = nullptr;
    check(f.resolve(&why), "exact witnessed image resolves");
    for (const auto &w : pins::kWitnesses) {
        uint8_t old = f.image[w.rva];
        f.image[w.rva] ^= 1;
        GameScaleBinding bad{};
        check(!resolveBindings({f.image, pins::kImageSize}, &bad, &why) && !bad.valid,
              "every independently corrupted witness refuses");
        f.image[w.rva] = old;
    }
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS64 *>(f.image + 0x80);
    ++nt->FileHeader.TimeDateStamp;
    check(!f.resolve(&why), "unknown build refuses");
    --nt->FileHeader.TimeDateStamp;
    check(f.resolve(&why), "supported identity restored");
    auto registration = uintptr_t(f.image) + pins::kWrapper;
    put(uintptr_t(f.image), pins::kRegistrationTable, uintptr_t(0));
    check(!f.resolve(&why), "requester registration pointer corruption refuses");
    put(uintptr_t(f.image), pins::kRegistrationTable, registration);
    check(f.resolve(&why), "registration restored");
    LiveOwner o{};
    check(f.owner(&o, &why) && o.mode == 0 && o.ssMax == std::numeric_limits<float>::max(),
          "flat owner and uncapped valid bounds");
    check(!readLiveOwner(f.binding, 1, &o, &why), "invalid graph contained by SEH");
    check(f.owner(&o, &why), "refresh owner");
    ScaleAction a{};
    a.owner = o;
    a.ss = {true, o.ss, .75f};
    auto r = applyScaleAction(f.binding, a);
    check(r.success && r.ssWritten && r.dirtyWritten && r.actualValid && r.actual.ss == .75f,
          "flat checked write and actual receipt");
    a.owner = r.actual;
    a.ss = {true, r.actual.ss, -.5f};
    r = applyScaleAction(f.binding, a);
    check(!r.success && !r.ssWritten, "out of bound preflight has no writes");
    a.ss = {true, a.owner.ss, .5f};
    auto savedContext = uintptr_t(f.context.data());
    put(uintptr_t(f.renderer.data()), pins::kRendererContext, uintptr_t(f.settings.data()));
    check(!applyScaleAction(f.binding, a).success, "replaced context refuses stale-owner write");
    put(uintptr_t(f.renderer.data()), pins::kRendererContext, savedContext);
    put(uintptr_t(f.context.data()), pins::kSs, .8f);
    r = applyScaleAction(f.binding, a);
    check(!r.success && !r.ssWritten, "new genuine value wins stale expected write");
    put(uintptr_t(f.context.data()), pins::kMode, 4u);
    check(f.owner(&o, &why) && o.mode == 4, "copied live VR mode");
    a = {};
    a.owner = o;
    a.contextHmd = {true, o.hmd, .5f};
    a.managerHmd = {true, o.managerHmd, .6f};
    r = applyScaleAction(f.binding, a);
    check(r.success && r.hmdWritten && r.managerWritten && r.actual.hmd == .5f && r.actual.managerHmd == .6f,
          "distinct HMD targets use real checked manager setter");
    put(uintptr_t(f.manager.data()), 0, uintptr_t(f.image) + pins::kManagerVtable + 8);
    check(!f.owner(&o, &why), "changed manager vtable refuses");
    put(uintptr_t(f.manager.data()), 0, uintptr_t(f.image) + pins::kManagerVtable);
    put(uintptr_t(f.settings.data()), pins::kSettingsManager, uintptr_t(0));
    check(!f.owner(&o, &why), "lost manager alias refuses");
    put(uintptr_t(f.settings.data()), pins::kSettingsManager, uintptr_t(f.manager.data()));
    put(uintptr_t(f.context.data()), pins::kMode, 7u);
    check(!f.owner(&o, &why), "unknown mode refuses");
}
extern "C" {
extern void *g_gameRenderScaleOriginal;
void gameRenderScaleTestInvoke(void *);
void gameRenderScaleTestOriginal();
void gameRenderScaleTestClobber();
__declspec(align(16)) uint8_t g_scaleExpectedFx[512] = {}, g_scaleActualFx[512] = {};
uint64_t g_scaleActualRegs[10] = {};
uint64_t g_scaleActualNonvol[8] = {};
uint64_t g_scaleCallbackAlignment = 0;
uintptr_t g_scaleTestTarget = 0;
}
unsigned callbacks = 0;
bool closeDuringCallback = false;
extern "C" void gameRenderScaleEntry();
void unwindTest() {
    CONTEXT context{};
    RtlCaptureContext(&context);
    bool sawBridge = false, sawCaller = false;
    for (unsigned i = 0; i < 16 && context.Rip; ++i) {
        DWORD64 imageBase = 0;
        auto *entry = RtlLookupFunctionEntry(context.Rip, &imageBase, nullptr);
        if (!entry)
            break;
        const uintptr_t start = uintptr_t(imageBase + entry->BeginAddress);
        sawBridge |= start == uintptr_t(&gameRenderScaleEntry);
        sawCaller |= start == uintptr_t(&gameRenderScaleTestInvoke);
        PVOID handler = nullptr;
        DWORD64 frame = 0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.Rip, entry, &context, &handler, &frame, nullptr);
        if (sawCaller)
            break;
    }
    check(sawBridge && sawCaller, "Windows unwinds callback through bridge to original caller");
}
void onEntry(uintptr_t r) noexcept {
    ++callbacks;
    check(r == 0x12345678, "callback receives original renderer");
    unwindTest();
    if (closeDuringCallback)
        closeHookAdmission();
    gameRenderScaleTestInvoke(reinterpret_cast<void *>(g_scaleTestTarget));
    gameRenderScaleTestClobber();
}
void abiTests() {
    Fixture f;
    const char *why = nullptr;
    check(f.resolve(&why), "ABI image resolves");
    uint8_t *tail = f.image + pins::kRequester + 18;
    const uint8_t pops[] = {0x41, 0x5f, 0x41, 0x5e, 0x41, 0x5d, 0x41, 0x5c, 0x5f, 0x5e, 0x5d,
                            0x48, 0x8b, 0x5c, 0x24, 0x18, 0xff, 0x25, 0,    0,    0,    0};
    std::memcpy(tail, pops, sizeof(pops));
    put(uintptr_t(tail), sizeof(pops), uintptr_t(&gameRenderScaleTestOriginal));
    struct Preparation {
        uint8_t *target;
        bool sawUnpatchedOriginal = false;
    } preparation{f.image + pins::kRequester};
    edvr::CodeHook refusedPreparation;
    void *unpublished = nullptr;
    const auto rejectPrepared = [](void *trampoline, void *opaque) noexcept {
        auto *state = static_cast<Preparation *>(opaque);
        state->sawUnpatchedOriginal = trampoline && state->target[0] == pins::kRequesterBytes[0];
        return false;
    };
    uint8_t originalEntry[8]{};
    std::memcpy(originalEntry, preparation.target, sizeof(originalEntry));
    check(!refusedPreparation.install(preparation.target, preparation.target + 0x200, &unpublished,
                                      "reject preparation", rejectPrepared, &preparation) &&
              preparation.sawUnpatchedOriginal && !refusedPreparation.installed() && !unpublished && callbacks == 0 &&
              !std::memcmp(originalEntry, preparation.target, sizeof(originalEntry)),
          "real Prepare refusal preserves entry bytes, no installation or callback");
    auto *hook = new edvr::CodeHook();
    check(!installCheckedHook(*hook, f.binding, onEntry, false, &why) && f.image[pins::kRequester] == 0x48,
          "unpinned module refuses before patch publication");
    f.image[pins::kRequester] = 0xE9;
    check(!installCheckedHook(*hook, f.binding, onEntry, true, &why) && why && *why &&
              f.image[pins::kRequester] == 0xE9 && !g_gameRenderScaleOriginal,
          "real CodeHook refusal retains original bytes and clears forwarding");
    f.image[pins::kRequester] = pins::kRequesterBytes[0];
    const bool installed = installCheckedHook(*hook, f.binding, onEntry, true, &why);
    check(installed, "real CodeHook prepared forwarding");
    if (!installed) {
        std::printf("hook refusal: %s\n", why ? why : "unspecified");
        delete hook;
        return;
    }
    std::printf("ABI requester=%p bridge=%p\n", f.image + pins::kRequester,
                reinterpret_cast<void *>(&gameRenderScaleEntry));
    gameRenderScaleTestInvoke(f.image + pins::kRequester);
    check(callbacks == 1, "bridge callback executes once including recursive requester");
    check(g_scaleActualRegs[9] == 8 && g_scaleCallbackAlignment == 8, "original and callback ABI stack alignment");
    for (unsigned i = 0; i < 8; ++i)
        check(g_scaleActualNonvol[i] == 0x12345000 + i, "nonvolatile integer register survives bridge");
    check(g_scaleActualRegs[0] == 0x12345678 && g_scaleActualRegs[1] == 0x22334455 &&
              g_scaleActualRegs[2] == 0x33445566 && g_scaleActualRegs[3] == 0x44556677 &&
              g_scaleActualRegs[4] == 0x55667788 && g_scaleActualRegs[5] == 0x66778899,
          "volatile integer arguments survive callback");
    check((g_scaleActualRegs[6] & 0x8d5) == 0x845, "entry flags preserved");
    check(g_scaleActualRegs[7] == 0x1122334455667788ull, "stack argument preserved");
    check(!std::memcmp(g_scaleExpectedFx, g_scaleActualFx, 32) &&
              !std::memcmp(g_scaleExpectedFx + 160, g_scaleActualFx + 160, 256),
          "FP SSE MXCSR restored");
    check(g_scaleActualRegs[8] == 0xaabbccddeeff0011ull, "original return preserved");
    closeDuringCallback = true;
    gameRenderScaleTestInvoke(f.image + pins::kRequester);
    check(callbacks == 2, "shutdown during in-flight callback returns through retained forwarding");
    gameRenderScaleTestInvoke(f.image + pins::kRequester);
    check(callbacks == 2, "shutdown closes callback admission and retains forwarding");
    int32_t displacement = 0;
    std::memcpy(&displacement, f.image + pins::kRequester + 1, sizeof(displacement));
    const uintptr_t relay = uintptr_t(f.image + pins::kRequester + 5) + displacement;
    MEMORY_BASIC_INFORMATION region{};
    check(VirtualQuery(reinterpret_cast<void *>(relay), &region, sizeof(region)) && region.State == MEM_COMMIT &&
              region.Protect == PAGE_EXECUTE_READ,
          "shutdown retains executable read-only relay");
    check(VirtualQuery(g_gameRenderScaleOriginal, &region, sizeof(region)) && region.State == MEM_COMMIT &&
              region.Protect == PAGE_EXECUTE_READ && hook->installed(),
          "shutdown retains original trampoline and installed entry");
    // Installed targets and hook objects deliberately outlive all in-flight
    // bridge calls.
    f.image = nullptr;
}
int main(int argc, char **argv) {
    if (argc == 2 && !std::strcmp(argv[1], "--dry-run")) {
        std::puts("game_render_scale_test: dry-run, no allocations or writes");
        return 0;
    }
    if (argc != 2)
        return 2;
    if (!std::strcmp(argv[1], "--self-test")) {
        bindingTests();
        scale_policy_test::run();
    }
    else if (!std::strcmp(argv[1], "--abi-test"))
        abiTests();
    else
        return 2;
    std::printf("game_render_scale_test: %u checks %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
