#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace skylanders {

class CemuSettingsXml {
public:
    struct GraphicPackSelection {
        bool disabled = false;
        std::map<std::string, std::string> presets;
    };

    explicit CemuSettingsXml(std::filesystem::path path);

    void loadOrCreate();
    void save() const;

    void setContentValue(const std::vector<std::string>& path, const std::string& value);
    std::optional<std::string> contentValue(const std::vector<std::string>& path) const;
    void enableGraphicPack(const std::filesystem::path& normalizedPackPath);
    void setGraphicPackPreset(
        const std::filesystem::path& normalizedPackPath,
        const std::string& category,
        const std::string& preset);

private:
    static std::string defaultSettingsXml();

    std::filesystem::path path_;
    std::string rawXml_;
    std::map<std::string, std::string> values_;
    std::map<std::string, GraphicPackSelection> graphicPacks_;
};

class IniDocument {
public:
    explicit IniDocument(std::filesystem::path path);

    void loadIfExists();
    void set(const std::string& section, const std::string& key, const std::string& value);
    void erase(const std::string& section, const std::string& key);
    void save() const;

private:
    std::filesystem::path path_;
    std::map<std::string, std::map<std::string, std::string>> sections_;
    std::vector<std::string> leadingComments_;
};

std::string titleIdToHex(std::uint64_t titleId);

struct GraphicPackCandidate {
    struct Preset {
        std::string category;
        std::string name;
    };

    std::filesystem::path rulesPath;
    std::filesystem::path configPath;
    std::string folderName;
    std::string definitionName;
    std::string virtualPath;
    std::string description;
    std::vector<std::uint64_t> titleIds;
    std::vector<Preset> presets;
    std::vector<std::string> presetNames;
    int score = 0;
};

std::vector<GraphicPackCandidate> scanCemuGraphicPacks(const std::filesystem::path& cemuRoot);

std::optional<GraphicPackCandidate> findBestGraphicPack(
    const std::filesystem::path& cemuRoot,
    std::span<const std::string_view> gameNeedles,
    std::span<const std::string_view> requiredNeedles,
    std::span<const std::string_view> optionalNeedles,
    std::optional<std::uint64_t> titleId = std::nullopt);

std::optional<std::string> findBestPreset(
    const GraphicPackCandidate& pack,
    std::span<const std::string_view> requiredNeedles,
    std::span<const std::string_view> optionalNeedles);

std::optional<GraphicPackCandidate::Preset> findBestPresetSelection(
    const GraphicPackCandidate& pack,
    std::span<const std::string_view> requiredNeedles,
    std::span<const std::string_view> optionalNeedles);

} // namespace skylanders
