// Formatting consumes copied controller state only; it never queries the game or engine.
#pragma once
#include "game_render_scale_policy.h"
#include <cstdio>
#include <string>
namespace edvr {
inline const char* renderScaleStateName(renderscale::State state) {
    using renderscale::State;
    switch (state) {
        case State::Game: return "Game";
        case State::PendingGame: return "Pending game resize";
        case State::PendingBoundary: return "Pending render boundary";
        case State::AwaitingFsr: return "Awaiting FSR frames";
        case State::Applied: return "Applied (submitted)";
        case State::Refused: return "Refused";
        case State::Restoring: return "Restoring";
    }
    return "Unknown";
}
inline const char* renderScalePresetName(fsr4_quality::Preset preset) {
    using fsr4_quality::Preset;
    switch (preset) {
        case Preset::Game: return "Game";
        case Preset::Native: return "Native";
        case Preset::Quality: return "Quality";
        case Preset::Balanced: return "Balanced";
        case Preset::Performance: return "Performance";
        case Preset::UltraPerformance: return "Ultra Performance";
        case Preset::Custom: return "Custom";
        case Preset::Invalid: return "Invalid";
    }
    return "Invalid";
}
inline std::string renderScaleDimensions(const renderscale::Snapshot& s, const Fsr4DispatchStatus* observed = nullptr) {
    char text[160];
    for (unsigned slot = 0; slot < (s.requested.vr ? 3u : 1u); ++slot) {
        const auto& d = s.slots[slot];
        if (!d.sequence || d.epoch != s.epoch) continue;
        std::snprintf(text, sizeof(text), "%s %s %ux%u -> %ux%u.",
                      d.outcome == Fsr4DispatchOutcome::Submitted ? "Submitted" : "Attempted",
                      s.requested.vr ? (slot == 0 ? "left" : slot == 1 ? "right" : "screen") : "flat",
                      d.inW, d.inH, d.outW, d.outH);
        return text;
    }
    // Backend observations remain useful in Game mode; they do not acknowledge
    // a managed request or change its state to Applied.
    if (observed && (s.state == renderscale::State::Game || !s.expectedOutW || !s.expectedOutH)) {
        for (unsigned slot = 0; slot < (s.requested.vr ? 3u : 1u); ++slot) {
            const auto& d = observed[slot];
            if (!d.sequence || d.outcome != Fsr4DispatchOutcome::Submitted || !d.inW || !d.inH || !d.outW || !d.outH) continue;
            std::snprintf(text, sizeof(text), "%s %s %ux%u -> %ux%u (FSR submitted).",
                d.epoch == s.epoch ? "Observed" : "Historical FSR",
                s.requested.vr ? (slot == 0 ? "left" : slot == 1 ? "right" : "screen") : "flat",
                d.inW, d.inH, d.outW, d.outH);
            return text;
        }
    }
    if (!s.expectedOutW || !s.expectedOutH) return "Observed sizes pending.";
    std::snprintf(text, sizeof(text), "Expected %ux%u -> %ux%u.", s.expectedInW, s.expectedInH,
                  s.expectedOutW, s.expectedOutH);
    return text;
}
inline std::string renderScaleSummary(const renderscale::Snapshot& s, const Fsr4DispatchStatus* observed = nullptr) {
    char text[1200];
    std::snprintf(text, sizeof(text), "Requested %s: %s. %s",
                  renderScalePresetName(s.requested.quality.preset), renderScaleStateName(s.state), s.reason);
    std::string result = text;
    result += " " + renderScaleDimensions(s, observed);
    if (s.actualValid) {
        std::snprintf(text, sizeof(text), " Actual game %s %.6g.", s.actual.mode ? "HMD" : "SS",
                      s.actual.mode ? s.actual.hmd : s.actual.ss);
        result += text;
    } else result += " Actual game scale unavailable.";
    return result;
}
inline std::string renderScaleStatus(const renderscale::Snapshot& s, const Fsr4DispatchStatus* observed = nullptr) {
    std::string result = renderScaleSummary(s, observed);
    for (unsigned slot = 0; slot < (s.requested.vr ? 3u : 1u); ++slot) {
        const auto& d = s.slots[slot];
        if (!d.sequence) continue;
        const char* route = s.requested.vr ? (slot == 0 ? "Left eye" : slot == 1 ? "Right eye" : "VR screen/world")
                                         : (slot == 0 ? "Flat" : "Other route");
        char text[900];
        std::snprintf(text, sizeof(text), " %s: %s %s %ux%u -> %ux%u. %s", route,
                      d.epoch == s.epoch ? "current" : "historical",
                      d.outcome == Fsr4DispatchOutcome::Submitted ? "submitted" :
                          d.outcome == Fsr4DispatchOutcome::Failed ? "failed" : "pending",
                      d.inW, d.inH, d.outW, d.outH, d.reason);
        result += text;
    }
    return result;
}
} // namespace edvr
