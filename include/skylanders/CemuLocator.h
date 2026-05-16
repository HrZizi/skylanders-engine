#pragma once

#include "skylanders/RuntimeModes.h"

#include <filesystem>
#include <optional>
#include <string>

namespace skylanders {

enum class CemuSource {
    BundledSubmodule,
    PersistedExternal,
    PortableLocal,
    LegacyFallback,
    PickedExternal,
};

struct CemuResolution {
    std::filesystem::path cemuRoot;
    std::filesystem::path cemuExe;
    CemuSource source = CemuSource::PickedExternal;
};

struct CemuValidationResult {
    bool valid = false;
    std::string reason;
};

struct SetupConfig {
    std::optional<std::filesystem::path> externalCemuExePath;
    std::optional<bool> fullscreen;
    std::optional<CemuBackendMode> cemuBackendMode;
    std::optional<OverlayBackendMode> overlayBackendMode;

    static SetupConfig load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;
};

struct CemuLocatorConfig {
    std::filesystem::path projectRoot;
    std::filesystem::path exePath;
    std::filesystem::path setupConfigFile;
    std::filesystem::path legacyCemuExe = L"E:\\Cemu_2.6\\Cemu.exe";
};

class CemuLocator {
public:
    explicit CemuLocator(CemuLocatorConfig config);

    std::optional<CemuResolution> resolve();
    std::optional<CemuResolution> promptAndPersist(void* ownerWindow);
    CemuValidationResult validateCemuExe(const std::filesystem::path& path) const;

    static std::string sourceToString(CemuSource source);

private:
    std::optional<CemuResolution> validateAs(const std::filesystem::path& path, CemuSource source) const;
    void clearPersisted() const;
    void persistExternal(const std::filesystem::path& cemuExe) const;

    CemuLocatorConfig config_;
};

} // namespace skylanders
