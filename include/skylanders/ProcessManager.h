#pragma once

#include "skylanders/GameCatalog.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace skylanders {

enum class ResolutionPreset {
    Native720p,
    TwoK,
    FourK,
};

struct GameLaunchSpec {
    GameId game = GameId::Unknown;
    std::wstring displayName;
    std::filesystem::path stubPath;
    std::filesystem::path gamePath;
    std::optional<std::uint64_t> titleId;
    std::wstring region;
    std::optional<GameImageSource> imageSource;
};

struct ProcessManagerConfig {
    std::filesystem::path cemuRoot = L"E:\\Cemu_2.6";
    std::filesystem::path runtimeRoot = L"E:\\skylanders_port\\runtime";
    std::filesystem::path figureLibraryRoot = L"E:\\skylanders_port\\figures";
    std::filesystem::path keysPath;
    bool fullscreen = true;
    std::vector<GameLaunchSpec> games;
};

struct AppliedCemuSetting {
    std::string key;
    std::string value;
};

struct GraphicPackSelectionReport {
    std::string label;
    bool selected = false;
    bool important = false;
    std::string packName;
    std::string configPath;
    std::string presetCategory;
    std::string preset;
    std::string warning;
};

struct CemuPreparationReport {
    GameId game = GameId::Unknown;
    std::filesystem::path settingsPath;
    std::filesystem::path gameProfilePath;
    std::filesystem::path player1ProfilePath;
    std::filesystem::path player2ProfilePath;
    std::vector<AppliedCemuSetting> appliedSettings;
    std::vector<GraphicPackSelectionReport> graphicPacks;
    std::vector<std::string> warnings;
};

struct RunningProcess {
    unsigned long processId = 0;
    void* processHandle = nullptr;
};

class ProcessManager {
public:
    explicit ProcessManager(ProcessManagerConfig config);

    static void validateKeysFile(const std::filesystem::path& path);

    GameLaunchSpec resolveFromStub(const std::filesystem::path& stubPath) const;
    RunningProcess launchFromStub(const std::filesystem::path& stubPath);
    RunningProcess launch(const GameLaunchSpec& spec);
    CemuPreparationReport prepareForLaunchDiagnostics(const GameLaunchSpec& spec);
    CemuPreparationReport prepareForEmbeddedDiagnostics(const GameLaunchSpec& spec);
    const CemuPreparationReport& lastPreparationReport() const;

    static std::string preparationReportJson(const CemuPreparationReport& report);

private:
    void validateCemuInstall() const;
    void validateCemuLayout() const;
    CemuPreparationReport prepareCemuEnvironment(const GameLaunchSpec& spec) const;
    void applyGlobalCemuSettings(const GameLaunchSpec& spec, CemuPreparationReport& report) const;
    void applyGameProfileOverrides(const GameLaunchSpec& spec, CemuPreparationReport& report) const;
    void isolateControllerProfiles(const GameLaunchSpec& spec, CemuPreparationReport& report) const;
    void ensureRuntimeLayout(const GameLaunchSpec& spec) const;
    void installKeysFile() const;
    void persistPreparationReport(const CemuPreparationReport& report) const;
    ResolutionPreset chooseResolutionPreset() const;

    RunningProcess spawnCemu(const GameLaunchSpec& spec) const;

    ProcessManagerConfig config_;
    CemuPreparationReport lastPreparationReport_;
};

} // namespace skylanders
