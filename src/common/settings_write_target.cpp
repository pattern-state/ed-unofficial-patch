#include "settings_write_target.h"
#include "runtime_profile.h"
#include "iniedit.h"
namespace edvr {
bool readWriteSource(const SettingsWriteTarget& target, std::string* source,
                     bool* destinationExists, std::string* error) {
    *destinationExists = false;
    if (!target.bound()) { *error = "settings write has no bound destination"; return false; }
    FileState state = readBoundedFile(target.path, 4u << 20, *source);
    *destinationExists = state != FileState::Absent;
    if (state == FileState::Absent && target.allowSeed && !target.fallbackSource.empty())
        state = readBoundedFile(target.fallbackSource, 4u << 20, *source);
    if (state != FileState::Read) {
        *error = runtimeProfileUtf8(target.leaf) + " could not be read";
        return false;
    }
    return true;
}
bool writeSettingsPairs(const SettingsWriteTarget& target,
    const std::vector<std::pair<std::string, std::string>>& pairs,
    SettingsWriteSideEffects& sideEffects, std::string* error) {
    std::string source;
    bool exists = false;
    if (!readWriteSource(target, &source, &exists, error)) return false;
    if (exists) sideEffects.backup(target);
    MergeReport report;
    const std::string leaf = runtimeProfileUtf8(target.leaf);
    const std::string updated = mergeIni(source, source, &source, pairs, &report, leaf.c_str());
    std::wstring why;
    if (!writeFileAtomic(target.path, updated, &why)) {
        *error = leaf + ": " + runtimeProfileUtf8(why);
        return false;
    }
    sideEffects.mirror(target, updated);
    return true;
}
} // namespace edvr
