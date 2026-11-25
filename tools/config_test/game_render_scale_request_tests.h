#pragma once
#include "../../src/common/config.h"
#include "../../src/d3d11/game_render_scale_policy.h"
namespace edvr {
template<class Check>
void gameRenderScaleRequestTests(Check&& check) {
    using namespace renderscale;
    Config& cfg = Config::get();
    const auto savedProfile = runtimeProfileValue();
    const auto savedMode = cfg.configuredTemporalMode();
    const bool savedOverride = cfg.hasRuntimeOverride("fix.temporal_aa");
    const auto savedRequested = cfg.requestedTemporalMode();
    const auto savedQuality = cfg.getString("fix.fsr4_quality", "game");
    const auto savedCustom = cfg.getString("fix.fsr4_render_scale", "0.75");
    cfg.clearRuntimeOverride("fix.temporal_aa");
    runtimeProfileSet(RuntimeProfile::Flat);
    cfg.set("fix.temporal_aa", "fsr4");
    cfg.set("fix.fsr4_quality", "custom");
    cfg.set("fix.fsr4_render_scale", "0.75");
    check(cfg.getString("fix.temporal_aa", "missing") == "off" && cfg.requestedTemporalMode() == "fsr4",
          "flat Config intentionally filters generic AA while retaining requested FSR4");
    RequestIntent intent;
    check(readRequestIntent(cfg, false, true, intent) && intent.fsrEnabled && !intent.vr &&
          intent.quality.preset == fsr4_quality::Preset::Custom && intent.quality.customScale == .75f,
          "production request reader enables flat FSR4 from real Config");
    intent.sequence = 1;
    intent.profileGeneration = 1;
    ScaleController controller;
    controller.publishRequest(intent);
    check(controller.snapshot().state == State::PendingGame,
          "flat persisted FSR4 reaches PendingGame instead of inert Game");
    cfg.setRuntimeOverride("fix.temporal_aa", "off");
    check(readRequestIntent(cfg, false, true, intent) && !intent.fsrEnabled,
          "F7 off override disables production render-quality intent");
    intent.sequence = 2;
    intent.profileGeneration = 1;
    controller.publishRequest(intent);
    check(controller.snapshot().state == State::Game, "F7 off releases flat intent without a managed receipt");
    cfg.clearRuntimeOverride("fix.temporal_aa");
    check(readRequestIntent(cfg, false, true, intent) && intent.fsrEnabled,
          "clearing F7 override restores persisted FSR4 intent");
    intent.sequence = 3;
    intent.profileGeneration = 1;
    controller.publishRequest(intent);
    check(controller.snapshot().state == State::PendingGame, "cleared F7 override requeues the managed request");
    cfg.set("fix.temporal_aa", "dlss");
    check(readRequestIntent(cfg, false, true, intent) && !intent.fsrEnabled,
          "non-FSR AA never enables render-quality control");
    cfg.set("fix.temporal_aa", "fsr4");
    check(readRequestIntent(cfg, false, false, intent) && !intent.fsrEnabled,
          "scope or graphics disable remains authoritative despite requested FSR4");
    runtimeProfileSet(RuntimeProfile::Vr);
    check(readRequestIntent(cfg, true, true, intent) && intent.fsrEnabled && intent.vr,
          "VR FSR4 intent retains existing requested-mode behaviour");
    struct ChangingConfig {
        uint32_t generation() const { return Config::get().generation(); }
        std::string getString(const char* key, const char* fallback) { return Config::get().getString(key, fallback); }
        std::string requestedTemporalMode() {
            const auto mode = Config::get().requestedTemporalMode();
            Config::get().set("fix.fsr4_render_scale", "0.8");
            return mode;
        }
    } changing;
    intent.sequence = 99;
    check(!readRequestIntent(changing, true, true, intent) && intent.sequence == 99,
          "changing Config during AA read retains the previously published request");
    cfg.clearRuntimeOverride("fix.temporal_aa");
    cfg.set("fix.temporal_aa", savedMode.c_str());
    cfg.set("fix.fsr4_quality", savedQuality.c_str());
    cfg.set("fix.fsr4_render_scale", savedCustom.c_str());
    if (savedOverride) cfg.setRuntimeOverride("fix.temporal_aa", savedRequested.c_str());
    runtimeProfileSet(savedProfile);
    // Runtime overrides notify the next reload even after their values are restored.
    // Consume this fixture's notification so the next file-reload fixture actually parses its edit.
    check(cfg.reloadIfChanged(), "request fixture consumes its runtime-override reload notification");
    check(!cfg.reloadIfChanged(), "request fixture leaves no pending reload notification for later file tests");
}
} // namespace edvr
