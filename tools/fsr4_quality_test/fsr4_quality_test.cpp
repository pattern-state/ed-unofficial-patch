// Pure CPU contracts for the shared FSR4 quality values.
#include "../../src/common/fsr4_quality.h"
#include <cstdio>
#include <cstring>
#include <limits>
using namespace edvr::fsr4_quality;
static int failures = 0;
static void check(bool value, const char* name) {
    printf("%s %s\n", value ? "ok" : "FAIL", name);
    if (!value) ++failures;
}
struct Reader {
    std::string preset = "custom", custom = "0.75";
    unsigned epoch = 0, mutations = 0;
    bool customPresent = true;
    unsigned generation() const { return epoch; }
    std::string getString(const char* key, const char* fallback) {
        if (std::strcmp(key, "fix.fsr4_quality") == 0) {
            if (mutations) { --mutations; ++epoch; }
            return preset;
        }
        return customPresent ? custom : fallback;
    }
};
static void selfTest() {
    const struct { const char* name; Preset preset; unsigned wide, high, eyeW, eyeH, oddW, oddH; } rows[] = {
        {"native", Preset::Native,5120,1440,2528,2704,2529,2705},
        {"quality",Preset::Quality,3413,960,1685,1802,1686,1803},
        {"balanced",Preset::Balanced,3011,847,1487,1590,1487,1591},
        {"performance",Preset::Performance,2560,720,1264,1352,1264,1352},
        {"ultra_performance",Preset::UltraPerformance,1706,480,842,901,843,901},
        {"custom",Preset::Custom,3840,1080,1896,2028,1896,2028}
    };
    for (const auto& row : rows) {
        check(parsePreset(row.name) == row.preset, row.name);
        const float scale = scaleFor(row.preset, 0.75f);
        unsigned out = 0;
        check(scaledDimension(5120, scale, out) && out == row.wide, "5120 width");
        check(scaledDimension(1440, scale, out) && out == row.high, "1440 height");
        check(scaledDimension(2528, scale, out) && out == row.eyeW, "Quest width");
        check(scaledDimension(2704, scale, out) && out == row.eyeH, "Quest height");
        check(scaledDimension(2529, scale, out) && out == row.oddW, "odd width");
        check(scaledDimension(2705, scale, out) && out == row.oddH, "odd height");
    }
    check(parsePreset("") == Preset::Game && parsePreset("game") == Preset::Game, "missing is Game");
    check(parsePreset("QUALITY") == Preset::Quality, "case insensitive preset");
    check(parsePreset("unknown") == Preset::Invalid, "unknown unmanaged");
    float value = 0.9f;
    check(parseCustomScale("", InputMode::Stored, value) && value == 0.75f, "empty stored defaults");
    check(!parseCustomScale("", InputMode::Typed, value), "empty typed refuses");
    const char* invalid[] = {"NaN","inf","-inf","1.1","0.3333333","0","0.75x","0.75  x","1e1000","1e-1000"};
    for (auto text : invalid) {
        value = 0.9f;
        check(!parseCustomScale(text, InputMode::Typed, value) && value == 0.9f, text);
    }
    check(parseCustomScale(" 0.75 \t", InputMode::Typed, value) && value == 0.75f, "outer whitespace");
    float low = 1.0f / 3.0f;
    bool roundtrips = true;
    for (int i = 0; i < 100; ++i) {
        float roundtrip = 0;
        roundtrips = roundtrips && parseCustomScale(formatCustomScale(low), InputMode::Typed, roundtrip) && low == roundtrip;
        low = roundtrip;
    }
    check(roundtrips, "100 minimum save/read round trips");
    check(parseCustomScale("1", InputMode::Typed, value) && value == 1.0f, "typed exact upper endpoint");
    check(steppedCustomScale(low, -1, 1) == low, "lower clamp");
    check(steppedCustomScale(low, 1, 1) == low + 0.01f, "no grid snap at minimum");
    check(steppedCustomScale(0.75f, 1, 5) == 0.8f, "Shift increment");
    check(steppedCustomScale(1.0f, 1, 5) == 1.0f, "upper clamp");
    check(steppedCustomScale(0.75f, -1, 5) == 0.7f, "Shift decrement");
    check(parseCustomScale(formatCustomScale(1.0f), InputMode::Typed, value) && value == 1.0f, "upper round trip");
    unsigned out = 77;
    check(scaledDimension(5120,0.99f,out) && out == 5068, "0.99 wide");
    check(scaledDimension(1440,0.99f,out) && out == 1425, "0.99 high");
    check(scaledDimension(2528,0.99f,out) && out == 2502, "0.99 eye width");
    check(scaledDimension(2704,0.99f,out) && out == 2676, "0.99 eye height");
    out = 77;
    check(!scaledDimension(0,1,out), "zero base refuses");
    check(!scaledDimension(1,0.5f,out), "zero result refuses");
    check(!scaledDimension(10,std::numeric_limits<float>::quiet_NaN(),out), "NaN refuses");
    check(!scaledDimension(10,std::numeric_limits<float>::infinity(),out), "infinite scale refuses");
    check(!scaledDimension(10,-1,out), "negative scale refuses");
    check(!scaledDimension(std::numeric_limits<unsigned>::max(),1,out), "float overflow refuses");
    check(out == 77, "dimension refusals preserve output");
    check(controlsEnabled("fsr4") && !controlsEnabled("dlss") && !controlsEnabled("off"), "engine enablement");
    std::string canonical;
    const char* reason = nullptr;
    check(editInactiveReason("fix.fsr4_quality", "off", "custom") != nullptr, "quality requires FSR4");
    check(editInactiveReason("fix.fsr4_render_scale", "fsr4", "quality") != nullptr, "scale requires Custom");
    check(editInactiveReason("fix.fsr4_render_scale", "fsr4", "custom") == nullptr, "custom scale enabled");
    check(validateEdit("fix.fsr4_quality", "QUALITY", "fsr4", "custom", "bad", canonical, &reason) &&
          canonical == "quality", "named preset canonicalizes despite malformed unused custom");
    check(!validateEdit("fix.fsr4_quality", "custom", "fsr4", "quality", "bad", canonical, &reason),
          "Custom activation refuses malformed saved scale");
    check(validateEdit("fix.fsr4_quality", "custom", "fsr4", "quality", "", canonical, &reason),
          "Custom activation keeps empty stored default");
    check(!validateEdit("fix.fsr4_render_scale", "0.75", "off", "custom", "0.75", canonical, &reason),
          "dependency rechecked after AA changes");
    check(!validateEdit("fix.fsr4_render_scale", "nan", "fsr4", "custom", "0.75", canonical, &reason),
          "nonfinite typed custom rejected");
    check(validateEdit("fix.fsr4_render_scale", " 0.75 ", "fsr4", "custom", "bad", canonical, &reason) &&
          canonical == "0.75", "valid typed custom repairs malformed remembered value");
    check(validateEdit("fix.unrelated", "untouched", "off", "game", "bad", canonical, &reason) &&
          canonical == "untouched", "unrelated edits unchanged");
    Reader reader;
    Request request;
    check(readRequest(reader,request) && request.valid && request.customScale == 0.75f, "adapter default");
    reader.customPresent = false;
    check(readRequest(reader,request) && request.valid && request.customScale == 0.75f, "missing custom uses raw-string default");
    reader.customPresent = true;
    reader.custom = "broken";
    check(readRequest(reader,request) && !request.valid && request.reason == Refusal::InvalidCustomScale, "malformed custom refuses");
    reader.preset = "quality";
    check(readRequest(reader,request) && request.valid, "unused malformed custom does not block named preset");
    reader.preset = "game";
    check(readRequest(reader,request) && request.valid, "unused malformed custom does not block Game");
    reader.preset = "typo";
    check(readRequest(reader,request) && !request.valid && request.reason == Refusal::UnknownPreset, "unknown reason retained");
    reader.preset = "custom"; reader.custom = "0.8"; reader.mutations = 1;
    check(readRequest(reader,request) && request.valid && request.customScale == 0.8f, "one generation retry");
    reader.custom = "0.9"; reader.mutations = 2;
    check(!readRequest(reader,request) && request.customScale == 0.8f, "churn preserves current request");
}
int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1],"--dry-run") == 0) { puts("CPU quality contracts; no writes"); return 0; }
    if (argc != 2 || std::strcmp(argv[1],"--self-test") != 0) { puts("usage: fsr4_quality_test --self-test | --dry-run"); return 2; }
    selfTest();
    printf("fsr4_quality_test: %d failures\n", failures);
    return failures ? 1 : 0;
}
