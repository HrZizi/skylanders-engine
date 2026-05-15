#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

namespace skylanders {

struct GraphicPackUpdateState {
    std::string status = "never_checked";
    std::string sourceUrl;
    std::string archiveHash;
    std::string lastCheckUtc;
    std::string lastInstallUtc;
    std::string lastError;
    int installedPackCount = 0;
};

struct GraphicPackInstallResult {
    bool ok = false;
    bool checked = false;
    std::string status;
    std::string sourceUrl;
    std::string archiveHash;
    std::string error;
    int installedPackCount = 0;
};

struct GraphicPackManagerConfig {
    std::filesystem::path cemuRoot;
    std::filesystem::path statePath;
    std::filesystem::path downloadRoot;
    std::string indexUrl = "https://cemu-project.github.io/cemu_graphic_packs/";
};

class GraphicPackManager {
public:
    explicit GraphicPackManager(GraphicPackManagerConfig config);

    GraphicPackUpdateState loadState() const;
    void saveState(const GraphicPackUpdateState& state) const;

    bool shouldAutoCheck(std::chrono::system_clock::time_point now = std::chrono::system_clock::now()) const;
    GraphicPackInstallResult update(bool force);
    GraphicPackInstallResult installZip(const std::filesystem::path& zipPath, const std::string& sourceUrl);

    static std::optional<std::string> findVersion6ZipUrl(const std::string& html, const std::string& baseUrl);
    static std::string stateToJson(const GraphicPackUpdateState& state);

private:
    GraphicPackManagerConfig config_;
};

} // namespace skylanders
