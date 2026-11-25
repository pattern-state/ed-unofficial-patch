#pragma once
#include "../../src/d3d11/menu_fsr4_quality.h"
#include "../../src/d3d11/game_render_scale_status.h"
namespace menu_fsr4_quality_test {
inline void run(void (*check)(bool, const char*)) {
    using namespace edvr;
    MenuFsr4Feedback feedback;
    std::string value;
    check(menuPrepareFsr4Edit("fix.fsr4_render_scale", "0.75", "fsr4", "custom", "0.75", feedback, value) &&
          value == "0.75", "F1 custom typed edit accepts fractional value");
    check(!menuPrepareFsr4Edit("fix.fsr4_render_scale", "0.75", "off", "custom", "0.75", feedback, value),
          "F2 AA change while typing refuses commit");
    check(std::string(menuFlatWriteStatus(feedback, "fix.fsr4_quality = custom", true)).find("not saved") != std::string::npos,
          "F3 refused edit cannot show saved after earlier write completion");
    check(!menuPrepareFsr4Edit("fix.fsr4_render_scale", "0.75", "fsr4", "quality", "0.75", feedback, value),
          "F4 preset change while typing refuses commit");
    check(!menuPrepareFsr4Edit("fix.fsr4_render_scale", "nan", "fsr4", "custom", "0.75", feedback, value),
          "F5 invalid typing retains explicit refusal");
    check(!menuPrepareFsr4Edit("fix.fsr4_render_scale", "0.75", "dlss", "custom", "0.75", feedback, value),
          "F6 reset obeys shared inactive predicate");
    const auto minimum = fsr4_quality::formatCustomScale(fsr4_quality::minimumCustomScale);
    check(menuFsr4Step(minimum, -1, 1, value) && value == minimum,
          "F7 held step at exact one-third never snaps below limit");
    check(menuFsr4Step("0.75", 1, 5, value) && value == fsr4_quality::formatCustomScale(.8f),
          "F8 held Shift step uses shared float rule");
    check(!menuFsr4Step("broken", 1, 1, value), "F9 malformed saved Custom does not silently step a default");
    uint64_t shown = 0;
    check(menuFsr4RevisionChanged(4, shown) && !menuFsr4RevisionChanged(4, shown) &&
          menuFsr4RevisionChanged(5, shown), "F10 flat and VR common revision check refreshes only changed status");
    menuFsr4ProfileChanged(feedback, shown);
    check(!feedback.refused && feedback.reason.empty() && shown == 0,
          "F17 profile transition clears outgoing refusal and cached revision");
    renderscale::Snapshot snapshot;
    snapshot.requested.quality.preset = fsr4_quality::Preset::Custom;
    snapshot.requested.vr = true;
    snapshot.state = renderscale::State::AwaitingFsr;
    snapshot.epoch = 2;
    snapshot.expectedInW = 1500;
    snapshot.expectedInH = 1200;
    snapshot.expectedOutW = 2000;
    snapshot.expectedOutH = 1600;
    snapshot.slots[0].sequence = 1;
    snapshot.slots[0].epoch = 2;
    snapshot.slots[0].outcome = Fsr4DispatchOutcome::Submitted;
    snapshot.slots[0].inW = 1500;
    snapshot.slots[0].inH = 1200;
    snapshot.slots[0].outW = 2000;
    snapshot.slots[0].outH = 1600;
    std::string text = renderScaleStatus(snapshot);
    check(text.find("Left eye: current submitted") != std::string::npos && text.find("Submitted left") != std::string::npos && text.find("Applied") == std::string::npos,
          "F11 left-only submission never formats as applied");
    snapshot.slots[1] = snapshot.slots[0];
    snapshot.slots[0] = {};
    text = renderScaleStatus(snapshot);
    check(text.find("Right eye: current submitted") != std::string::npos && text.find("Applied") == std::string::npos,
          "F12 right-only submission never formats as applied");
    snapshot.slots[2] = snapshot.slots[1];
    snapshot.slots[1] = {};
    text = renderScaleStatus(snapshot);
    check(text.find("VR screen/world: current submitted") != std::string::npos && text.find("Applied") == std::string::npos,
          "F13 world route is separately labelled and does not imply eyes");
    snapshot.slots[2].epoch = 1;
    text = renderScaleStatus(snapshot);
    check(text.find("historical submitted") != std::string::npos && text.find("current submitted") == std::string::npos,
          "F14 stale epoch explicitly historical");
    snapshot.state = renderscale::State::Restoring;
    snapshot.slots[0] = snapshot.slots[2];
    snapshot.slots[0].epoch = 2;
    snapshot.slots[0].outcome = Fsr4DispatchOutcome::Failed;
    text = renderScaleStatus(snapshot);
    check(text.find("Restoring") != std::string::npos && text.find("Attempted left") != std::string::npos && text.find("Left eye: current failed") != std::string::npos &&
          text.find("historical submitted") != std::string::npos, "F15 failed-after-success and restoration remain distinct");
    check(text.find("1500x1200 -> 2000x1600") != std::string::npos && text.find("Requested Custom") != std::string::npos,
          "F16 requested choice and expected dimensions present without tooltip");
}
} // namespace menu_fsr4_quality_test
