#include "../../src/d3d11/ui_game_resize.h"
#include <cstring>
#include <initializer_list>
#include <string>
namespace gameresize {
inline void testAll() {
    using namespace edvr;
    UiPanelPlan p{};
    check(uiGameResizeFlatPlan(1920, 1080, 3840, 2160, 1.25f, &p) && std::fabs(p.f - 0.4) < 1e-6,
          "game resize: intended flat half-scale prepares factor0.4 before scene follows");
    check(!uiGameResizeFlatPlan(1920, 1080, 0, 0, 1.25f, &p), "game resize: missing measured output defers");
    check(!uiGameResizeFlatPlan(512, 512, 3840, 2160, 1.25f, &p), "game resize: incompatible scene geometry defers");
    UiPanelInputs in{};
    in.outputW = 3000;
    in.fovTangent = 1.5f;
    in.trueTangent = 1.5f;
    in.target = 1.25f;
    in.displayW = 3840;
    in.screenW = 0;
    check(uiGameResizeVrPlan(in, 3000, 0.5f, 1.0f, &p) && std::fabs(p.lineF - 0.4) < 1e-6,
          "game resize: VR intended HMD and held SS use raw panel output basis");
    in.displayW = 6000;
    check(uiGameResizeVrPlan(in, 3000, 0.5f, 2.0f, &p) && p.largest <= 14336.001,
          "game resize: intended VR dimensions retain panel budget");
    check(!uiGameResizeVrPlan(in, 0, 0.5f, 1.0f, &p), "game resize: missing game-facing ask defers");
    check(!uiGameResizeVrPlan(in, 3000, NAN, 1.0f, &p), "game resize: invalid intended HMD defers");
    struct FinishEnv {
        uint64_t generation = 8;
        bool active = true, locked = false;
        unsigned restores = 0, finishes = 0;
        double factor = 0.4;
        void lock() { locked = true; }
        void unlock() { locked = false; }
        uint64_t resizeGeneration() const { return generation; }
        bool resizeInProgress() const { return active; }
        void restorePrevious(const UiGameResizeReceipt &receipt) {
            ++restores;
            factor = receipt.previousFactor;
        }
        void finishTransition() {
            ++finishes;
            active = false;
        }
    } env;
    UiGameResizeReceipt receipt{};
    receipt.active = true;
    receipt.generation = 8;
    receipt.previousFactor = 0.8;
    check(uiGameResizeFinishSection(receipt, false, env) && env.restores == 1 && env.factor == 0.8 && !env.active &&
              !env.locked,
          "game resize: no game write rolls panel factors back under float lock");
    env.active = true;
    env.factor = 0.4;
    check(uiGameResizeFinishSection(receipt, true, env) && env.restores == 1 && env.factor == 0.4 && !env.active,
          "game resize: partial or successful game write keeps prepared factor for checked restoration");
    env.active = true;
    env.generation = 9;
    check(!uiGameResizeFinishSection(receipt, false, env) && env.active && env.restores == 1 && !env.locked,
          "game resize: superseded rollback cannot restore another transition's divisors");
    struct SetterEnv {
        uint64_t generation = 10;
        bool active = true, locked = false;
        unsigned writes = 0;
        double factor = 0.8, line = 0.8;
        UiPublishedPlan pub{};
        SetterEnv() {
            pub.ready = true;
            pub.formula = 0.8;
            pub.base = 3840;
        }
        void lock() { locked = true; }
        void unlock() { locked = false; }
        bool publicationReady() const { return pub.ready; }
        void setPublicationReady(bool value) { pub.ready = value; }
        uint64_t resizeGeneration() const { return generation; }
        bool resizeInProgress() const { return active; }
        UiPublishedPlan published() const { return pub; }
        double factorNow() const { return factor; }
        double lineFactorNow() const { return line; }
        bool writeFactors(double f, double lineF, double) {
            if (active)
                return false;
            ++writes;
            factor = f;
            line = lineF;
            return true;
        }
        void seam(UiFlatSeam) {}
        void noteMove(bool, uint64_t) {}
        void restorePrevious(const UiGameResizeReceipt &receipt) {
            factor = receipt.previousFactor;
            line = receipt.previousLineFactor;
            pub.ready = receipt.previousReady;
        }
        void finishTransition() { active = false; }
    } setter;
    UiGameResizeReceipt prepared{};
    prepared.active = true;
    prepared.generation = 10;
    prepared.previousFactor = prepared.previousLineFactor = 0.8;
    setter.lock();
    uiGameResizeInvalidateSetterPublication(prepared, setter);
    setter.factor = setter.line = 0.4;
    setter.unlock();
    check(!uiPanelSetterSection(setter, 1.0f) && setter.factor == 0.4,
          "game resize: same SS during preparation cannot overwrite intended HMD factor");
    check(uiGameResizeFinishSection(prepared, true, setter) && !uiPanelSetterSection(setter, 1.0f) &&
              setter.factor == 0.4 && !setter.pub.ready,
          "game resize: after handoff same SS cannot republish stale pre-resize VR factor");
    setter.pub.formula = 0.4;
    setter.pub.ready = true;
    check(!uiPanelSetterSection(setter, 1.0f) && setter.factor == 0.4,
          "game resize: fresh frame publication permits setters with actual geometry");
    setter = SetterEnv{};
    setter.lock();
    uiGameResizeInvalidateSetterPublication(prepared, setter);
    setter.factor = setter.line = 0.4;
    setter.unlock();
    check(uiGameResizeFinishSection(prepared, false, setter) && setter.pub.ready && setter.factor == 0.8 &&
              !uiPanelSetterSection(setter, 1.0f),
          "game resize: zero-write rollback restores prior setter readiness and factors together");
    setter = SetterEnv{};
    setter.lock();
    uiGameResizeInvalidateSetterPublication(prepared, setter);
    setter.factor = setter.line = 0.4;
    setter.unlock();
    ++setter.generation;
    check(!uiGameResizeFinishSection(prepared, false, setter) && !setter.pub.ready && setter.factor == 0.4 &&
              setter.active,
          "game resize: stale rollback cannot revive prior publication");
    check(uiGameResizeMayPublish(7, 7, false), "game resize: current completed generation may publish");
    check(!uiGameResizeMayPublish(7, 8, false),
          "game resize: frame gathered before transition cannot overwrite prepared factor");
    check(!uiGameResizeMayPublish(8, 8, true), "game resize: in-flight game stores hold prepared factor");
}
} // namespace gameresize

namespace gameresize {
inline void testWiring(const std::string &scale, const std::string &surfaces) {
    const auto order = [](const std::string &text, std::initializer_list<const char *> needles) {
        size_t at = 0;
        for (const auto *needle : needles) {
            at = text.find(needle, at);
            if (at == std::string::npos)
                return false;
            at += std::strlen(needle);
        }
        return true;
    };
    check(order(scale, {"const float requested = x;", "vrSsaaHoldSetterValue(requested)",
                        "gameRenderScaleFlatSetterValue(context, requested, held)",
                        "guardedRun(setterBeforeThunk, self, x)", "orig(self, x)"}),
          "game resize: raw request precedes ordinary SS hold, managed override and panel preparation");
    check(order(surfaces, {"float hmdFileCached()", "float effectiveHmdQuality()",
                           "gameRenderScaleObservedHmd(&value) ? value : hmdFileCached()"}) &&
              surfaces.find("const float hmd = effectiveHmdQuality();") != std::string::npos &&
              surfaces.find("uiSurfacesHmdQuality() { return effectiveHmdQuality(); }") != std::string::npos,
          "game resize: CreateTexture2D basis and panel accessor share observed-HMD/file fallback owner");
}
} // namespace gameresize
