// Shared F8 edit preparation; the real menu uses these paths before queuing a captured write.
#pragma once
#include "../common/fsr4_quality.h"
#include "game_render_scale_status.h"
#include "flat_elite_settings.h"
#include "menu_panel.h"
#include <cstdint>
#include <string>
namespace edvr {
struct MenuFsr4Feedback {
    bool refused = false;
    std::string reason;
};
inline bool menuFsr4Key(const std::string& dotted) {
    return dotted == "fix.fsr4_quality" || dotted == "fix.fsr4_render_scale";
}
inline bool menuPrepareFsr4Edit(const std::string& dotted, const std::string& value,
                                const std::string& temporalMode, const std::string& preset,
                                const std::string& storedCustom, MenuFsr4Feedback& feedback,
                                std::string& canonical) {
    const char* why = nullptr;
    const bool allowed = fsr4_quality::validateEdit(dotted, value, temporalMode, preset, storedCustom, canonical, &why);
    if (!allowed) {
        feedback.refused = true;
        feedback.reason = why;
    }
    return allowed;
}
inline bool menuFsr4Step(const std::string& value, int direction, int multiplier, std::string& result) {
    float scale = 0;
    if (!fsr4_quality::parseCustomScale(value, fsr4_quality::InputMode::Stored, scale)) return false;
    result = fsr4_quality::formatCustomScale(fsr4_quality::steppedCustomScale(scale, direction, multiplier));
    return true;
}
inline const char* menuFlatWriteStatus(const MenuFsr4Feedback& feedback, const std::string& lastWrite, bool keysReady) {
    if (feedback.refused) return " | not saved";
    if (lastWrite.compare(0, 7, "FAILED:") == 0) return " | write failed";
    if (lastWrite.compare(0, 8, "writing ") == 0) return " | saving";
    if (!keysReady) return " | keyboard gate unavailable";
    return lastWrite.empty() ? "" : " | saved";
}
inline void menuFsr4ProfileChanged(MenuFsr4Feedback& feedback, uint64_t& shownRevision) {
    feedback = {};
    shownRevision = 0;
}
inline bool menuFsr4RevisionChanged(uint64_t revision, uint64_t& shownRevision) {
    if (revision == shownRevision) return false;
    shownRevision = revision;
    return true;
}
// One production composition path for both the real menu and raster regressions.
struct MenuQualityStatus {
    renderscale::Snapshot controller{};
    Fsr4DispatchStatus observed[3]{};
    bool flat = false, qualityPage = false, qualityHighlighted = false, aaHighlighted = false;
    std::string inactive, refusal, backend;
};
inline void menuComposeQualityStatus(MenuContent& c, const MenuQualityStatus& view,
                                    FlatMeasureFn measure, void* context) {
    const auto& snapshot = view.controller;
    if (view.qualityHighlighted) {
        // Flat already has persistent notes. Its hint explains inactivity only.
        if (view.flat) std::snprintf(c.hint, sizeof(c.hint), "%s", view.inactive.c_str());
        else std::snprintf(c.hint, sizeof(c.hint), "%s", view.inactive.empty() ? renderScaleStatus(snapshot).c_str() : view.inactive.c_str());
    }
    if (view.aaHighlighted)
        std::snprintf(c.hint, sizeof(c.hint), "%s", view.backend.c_str());
    if (!view.qualityPage && view.refusal.empty()) return;
    std::string status = !view.refusal.empty() ? "Not saved: " + view.refusal : renderScaleSummary(snapshot, view.observed);
    FlatSettingsWarning note;
    const int width = c.cardPx - 2 * (c.capPx * 8 / 10);
    flatWrapWarning(status, width, measure, context, &note);
    if (view.flat && note.count > kMenuRenderQualityLines) {
        status = std::string("Requested ") + renderScalePresetName(snapshot.requested.quality.preset) + ": " +
                 renderScaleStateName(snapshot.state) + ". " + renderScaleDimensions(snapshot, view.observed) + " Full reason on Status.";
        note = {};
        flatWrapWarning(status, width, measure, context, &note);
    }
    if (!view.flat) {
        if (!view.refusal.empty()) std::snprintf(c.hint, sizeof(c.hint), "Not saved: %s", view.refusal.c_str());
        else if (view.inactive.empty()) std::snprintf(c.hint, sizeof(c.hint), "Requested %s: %s.\n%s",
            renderScalePresetName(snapshot.requested.quality.preset), renderScaleStateName(snapshot.state), renderScaleDimensions(snapshot).c_str());
        note = {};
    }
    const int limit = view.flat ? kMenuRenderQualityLines : 2;
    for (int i = 0; i < note.count && i < limit && c.lineCount < kMenuMaxLines; ++i) {
        MenuLine& line = c.lines[c.lineCount++];
        std::strncpy(line.left, note.line[i], sizeof(line.left) - 1);
        line.style = kMenuNote;
    }
}
} // namespace edvr
