#pragma once

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>

namespace edvr { namespace fsr4_quality {

enum class Preset { Game, Native, Quality, Balanced, Performance, UltraPerformance, Custom, Invalid };
enum class InputMode { Stored, Typed };
enum class Refusal { None, UnknownPreset, InvalidCustomScale };

constexpr float minimumCustomScale = 1.0f / 3.0f;
constexpr float defaultCustomScale = 0.75f;

struct Request {
    Preset preset = Preset::Game;
    float customScale = defaultCustomScale;
    bool valid = true;
    Refusal reason = Refusal::None;
};

inline std::string trimmed(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n\f\v");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n\f\v") - first + 1);
}

inline std::string lowerAscii(std::string value) {
    for (char& c : value) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    return value;
}

inline Preset parsePreset(const std::string& text) {
    const auto value = lowerAscii(trimmed(text));
    if (value.empty() || value == "game") return Preset::Game;
    if (value == "native") return Preset::Native;
    if (value == "quality") return Preset::Quality;
    if (value == "balanced") return Preset::Balanced;
    if (value == "performance") return Preset::Performance;
    if (value == "ultra_performance") return Preset::UltraPerformance;
    if (value == "custom") return Preset::Custom;
    return Preset::Invalid;
}

inline bool parseCustomScale(const std::string& text, InputMode mode, float& result) {
    const auto value = trimmed(text);
    if (value.empty()) {
        if (mode == InputMode::Typed) return false;
        result = defaultCustomScale;
        return true;
    }
    // A classic locale keeps saved decimals independent of the Windows locale.
    std::istringstream stream(value);
    stream.imbue(std::locale::classic());
    float parsed = 0;
    stream >> std::noskipws >> parsed;
    if (stream.fail() || !stream.eof() || !std::isfinite(parsed) ||
        parsed < minimumCustomScale || parsed > 1.0f) return false;
    result = parsed;
    return true;
}

inline std::string formatCustomScale(float scale) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<float>::max_digits10) << scale;
    return stream.str();
}

inline float steppedCustomScale(float value, int direction, int multiplier = 1) {
    return std::clamp(value + static_cast<float>(direction) * 0.01f * static_cast<float>(multiplier),
                      minimumCustomScale, 1.0f);
}

inline float scaleFor(Preset preset, float customScale = defaultCustomScale) {
    switch (preset) {
    case Preset::Native: return 1.0f;
    case Preset::Quality: return 1.0f / 1.5f;
    case Preset::Balanced: return 1.0f / 1.7f;
    case Preset::Performance: return 0.5f;
    case Preset::UltraPerformance: return 1.0f / 3.0f;
    case Preset::Custom: return customScale;
    default: return 0.0f; // Game/invalid requests do not supply a managed scale.
    }
}

inline bool scaledDimension(unsigned base, float scale, unsigned& result) {
    if (!base || !std::isfinite(scale) || scale <= 0) return false;
    // Preserve the game's float multiply, then guard before integer truncation.
    const float product = static_cast<float>(base) * scale;
    if (!std::isfinite(product) || product < 1.0f ||
        static_cast<double>(product) > static_cast<double>(std::numeric_limits<unsigned>::max())) return false;
    result = static_cast<unsigned>(product);
    return true;
}

inline bool controlsEnabled(const std::string& temporalMode) {
    return lowerAscii(trimmed(temporalMode)) == "fsr4";
}

// The launcher supplies persisted AA; F8 supplies AA including runtime overrides.
inline const char* editInactiveReason(const std::string& dotted, const std::string& temporalMode,
                                      const std::string& preset) {
    if (dotted != "fix.fsr4_quality" && dotted != "fix.fsr4_render_scale") return nullptr;
    if (!controlsEnabled(temporalMode)) return "Select FSR4 anti-aliasing to change render quality.";
    if (dotted == "fix.fsr4_render_scale" && parsePreset(preset) != Preset::Custom)
        return "Choose Custom render quality to change its scale.";
    return nullptr;
}

inline bool validateEdit(const std::string& dotted, const std::string& value,
                         const std::string& temporalMode, const std::string& preset,
                         const std::string& storedCustom, std::string& canonical,
                         const char** why = nullptr) {
    const char* reason = editInactiveReason(dotted, temporalMode, preset);
    canonical = value;
    if (!reason && dotted == "fix.fsr4_quality") {
        const Preset selected = parsePreset(value);
        if (selected == Preset::Invalid) reason = "Choose a valid FSR4 quality preset.";
        else {
            float custom = defaultCustomScale;
            if (selected == Preset::Custom && !parseCustomScale(storedCustom, InputMode::Stored, custom))
                reason = "Custom scale is invalid; enter a value from 1/3 to 1.";
            canonical = lowerAscii(trimmed(value));
            if (canonical.empty()) canonical = "game";
        }
    } else if (!reason && dotted == "fix.fsr4_render_scale") {
        float scale = 0;
        if (!parseCustomScale(value, InputMode::Typed, scale))
            reason = "Custom scale needs a finite number from 1/3 to 1.";
        else canonical = formatCustomScale(scale);
    }
    if (why) *why = reason;
    return reason == nullptr;
}

// Reader is Config or a reader with the same generation/getString interface.
// False means a changing snapshot: retain the caller's current control and retry
// at the next boundary. A stable invalid value is published with its refusal.
template<class Reader>
bool readRequest(Reader& reader, Request& result) {
    for (unsigned attempt = 0; attempt != 2; ++attempt) {
        const auto generation = reader.generation();
        const auto preset = reader.getString("fix.fsr4_quality", "game");
        const auto custom = reader.getString("fix.fsr4_render_scale", "0.75");
        if (generation != reader.generation()) continue;
        Request candidate;
        candidate.preset = parsePreset(preset);
        const bool customValid = parseCustomScale(custom, InputMode::Stored, candidate.customScale);
        if (candidate.preset == Preset::Invalid) {
            candidate.valid = false;
            candidate.reason = Refusal::UnknownPreset;
        } else if (candidate.preset == Preset::Custom && !customValid) {
            candidate.valid = false;
            candidate.reason = Refusal::InvalidCustomScale;
        }
        result = candidate;
        return true;
    }
    return false;
}

}} // namespace edvr::fsr4_quality
