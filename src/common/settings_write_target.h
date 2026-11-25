// Immutable identity captured when a settings edit is made.
#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "install_scope.h"
namespace edvr {
struct SettingsWriteTarget {
    RuntimeProfile profile = RuntimeProfile::Invalid;
    uint64_t epoch = 0;
    std::wstring path, fallbackSource, leaf;
    bool allowSeed = false;
    bool bound() const { return epoch != 0 && !path.empty() && !leaf.empty(); }
};
inline bool sameWriteTarget(const SettingsWriteTarget& a, const SettingsWriteTarget& b) {
    return a.bound() && b.bound() && a.profile == b.profile && a.epoch == b.epoch &&
        a.path == b.path && a.fallbackSource == b.fallbackSource && a.leaf == b.leaf && a.allowSeed == b.allowSeed;
}
// Backup and mirror are safety copies; their existing best-effort policy is supplied by the caller.
// Neither callback selects or writes the authoritative settings document.
struct SettingsWriteSideEffects {
    virtual ~SettingsWriteSideEffects() = default;
    virtual void backup(const SettingsWriteTarget&) {}
    virtual void mirror(const SettingsWriteTarget&, const std::string&) {}
};
bool readWriteSource(const SettingsWriteTarget& target, std::string* source,
                     bool* destinationExists, std::string* error);
bool writeSettingsPairs(const SettingsWriteTarget& target,
    const std::vector<std::pair<std::string, std::string>>& pairs,
    SettingsWriteSideEffects& sideEffects, std::string* error);
} // namespace edvr
