#include "skylanders/ProcessManager.h"

#include "skylanders/CemuConfigEditors.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace skylanders {
namespace {

std::filesystem::path gameRuntimeRoot(const ProcessManagerConfig& config, GameId game) {
    return config.runtimeRoot / gameIdToString(game);
}

std::filesystem::path cemuSettingsPath(const ProcessManagerConfig& config) {
    return config.cemuRoot / L"settings.xml";
}

std::filesystem::path cemuControllerProfilesPath(const ProcessManagerConfig& config) {
    return config.cemuRoot / L"controllerProfiles";
}

std::filesystem::path cemuGameProfilesPath(const ProcessManagerConfig& config) {
    return config.cemuRoot / L"gameProfiles";
}

std::filesystem::path gameProfilePath(const ProcessManagerConfig& config, std::uint64_t titleId) {
    return cemuGameProfilesPath(config) / (titleIdToHex(titleId) + ".ini");
}

std::filesystem::path preparationReportPath(const ProcessManagerConfig& config) {
    return config.runtimeRoot / L"config" / L"last_launch_diagnostics.json";
}

std::filesystem::path gameMlcPath(const ProcessManagerConfig& config, GameId game) {
    return gameRuntimeRoot(config, game) / L"mlc01";
}

std::wstring quoteArg(const std::filesystem::path& path) {
    auto value = path.wstring();
    std::wstring escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back(L'"');
    for (wchar_t ch : value) {
        if (ch == L'"') {
            escaped += L"\\\"";
        } else {
            escaped.push_back(ch);
        }
    }
    escaped.push_back(L'"');
    return escaped;
}

void writeEmptyControllerProfile(const std::filesystem::path& path, const char* type) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("failed to write " + path.string());
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<emulated_controller>\n";
    out << "  <type>" << type << "</type>\n";
    out << "</emulated_controller>\n";
}

const wchar_t* playerProfileName(int player) {
    return player == 1 ? L"skylanders_player1" : L"skylanders_player2";
}

std::filesystem::path sharedControllerProfilesPath(const ProcessManagerConfig& config) {
    return config.runtimeRoot / L"controllerProfiles";
}

std::filesystem::path sharedPlayerProfilePath(const ProcessManagerConfig& config, int player) {
    return sharedControllerProfilesPath(config) / (std::wstring(playerProfileName(player)) + L".xml");
}

std::filesystem::path gamePlayerProfilePath(const ProcessManagerConfig& config, GameId game, int player) {
    return gameRuntimeRoot(config, game) / L"controllerProfiles" / (std::wstring(playerProfileName(player)) + L".xml");
}

void writeMapping(std::ostream& out, int mapping, int button) {
    out << "      <entry><mapping>" << mapping << "</mapping><button>" << button << "</button></entry>\n";
}

void writeDefaultKeyboardProProfile(const std::filesystem::path& path, const std::string& profileName) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("failed to write " + path.string());
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<emulated_controller>\n";
    out << "  <type>Wii U Pro Controller</type>\n";
    out << "  <profile>" << profileName << "</profile>\n";
    out << "  <controller>\n";
    out << "    <api>Keyboard</api>\n";
    out << "    <uuid>keyboard</uuid>\n";
    out << "    <display_name>Keyboard</display_name>\n";
    out << "    <axis><deadzone>0.25</deadzone><range>1</range></axis>\n";
    out << "    <rotation><deadzone>0.25</deadzone><range>1</range></rotation>\n";
    out << "    <trigger><deadzone>0.25</deadzone><range>1</range></trigger>\n";
    out << "    <mappings>\n";
    writeMapping(out, 1, 'Z');
    writeMapping(out, 2, 'X');
    writeMapping(out, 3, 'C');
    writeMapping(out, 4, 'V');
    writeMapping(out, 5, 'Q');
    writeMapping(out, 6, 'E');
    writeMapping(out, 7, '1');
    writeMapping(out, 8, '3');
    writeMapping(out, 9, VK_RETURN);
    writeMapping(out, 10, VK_BACK);
    writeMapping(out, 11, 'H');
    writeMapping(out, 12, VK_UP);
    writeMapping(out, 13, VK_DOWN);
    writeMapping(out, 14, VK_LEFT);
    writeMapping(out, 15, VK_RIGHT);
    writeMapping(out, 16, VK_LSHIFT);
    writeMapping(out, 17, VK_RSHIFT);
    writeMapping(out, 18, 'W');
    writeMapping(out, 19, 'S');
    writeMapping(out, 20, 'A');
    writeMapping(out, 21, 'D');
    writeMapping(out, 22, 'I');
    writeMapping(out, 23, 'K');
    writeMapping(out, 24, 'J');
    writeMapping(out, 25, 'L');
    out << "    </mappings>\n";
    out << "  </controller>\n";
    out << "</emulated_controller>\n";
}

void copyIfPresent(const std::filesystem::path& from, const std::filesystem::path& to) {
    if (!std::filesystem::exists(from)) {
        return;
    }

    std::filesystem::create_directories(to.parent_path());
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
}

bool isHex(char ch) {
    return (ch >= '0' && ch <= '9') ||
        (ch >= 'a' && ch <= 'f') ||
        (ch >= 'A' && ch <= 'F');
}

bool isKeySeparator(char ch) {
    return ch == ' ' || ch == '\t' || ch == '-' || ch == '_';
}

std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return {first, last};
}

std::string jsonEscape(std::string value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

bool lineContains128BitKey(const std::string& line) {
    for (std::size_t start = 0; start < line.size(); ++start) {
        if (!isHex(line[start])) {
            continue;
        }
        if (start > 0 && isHex(line[start - 1])) {
            continue;
        }

        std::size_t cursor = start;
        int hexDigits = 0;
        while (cursor < line.size() && hexDigits < 32) {
            if (isHex(line[cursor])) {
                ++hexDigits;
                ++cursor;
                continue;
            }
            if (isKeySeparator(line[cursor]) && hexDigits > 0) {
                ++cursor;
                continue;
            }
            break;
        }

        while (cursor < line.size() && isKeySeparator(line[cursor])) {
            ++cursor;
        }
        if (hexDigits == 32 && (cursor == line.size() || !isHex(line[cursor]))) {
            return true;
        }
    }

    return false;
}

void validateKeysFileFormat(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("keys.txt was not found at " + path.string());
    }

    bool foundKey = false;
    std::string line;
    int lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        const auto comment = line.find_first_of("#;");
        if (comment != std::string::npos) {
            line.resize(comment);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            continue;
        }
        if (!lineContains128BitKey(line)) {
            throw std::runtime_error("keys.txt has no 128-bit key at line " + std::to_string(lineNumber));
        }
        foundKey = true;
    }

    if (!foundKey) {
        throw std::runtime_error("keys.txt does not contain any 128-bit keys");
    }
}

std::string resolutionPresetName(ResolutionPreset preset) {
    switch (preset) {
    case ResolutionPreset::FourK: return "3840x2160";
    case ResolutionPreset::TwoK: return "2560x1440";
    case ResolutionPreset::Native720p: return "1280x720 (Default)";
    }
    return "1280x720 (Default)";
}

std::vector<std::string_view> gameNeedles(GameId game) {
    const auto* entry = findGameCatalogEntry(game);
    if (!entry) {
        return {};
    }
    std::vector<std::string_view> needles;
    needles.reserve(entry->searchTerms.size());
    for (const auto& term : entry->searchTerms) {
        needles.push_back(term);
    }
    return needles;
}

void recordSetting(CemuPreparationReport& report, std::string key, std::string value) {
    report.appliedSettings.push_back(AppliedCemuSetting{std::move(key), std::move(value)});
}

void setAndRecord(CemuSettingsXml& settings, CemuPreparationReport& report, const std::vector<std::string>& path, const std::string& value) {
    settings.setContentValue(path, value);
    std::string key;
    for (std::size_t i = 1; i < path.size(); ++i) {
        if (!key.empty()) {
            key += "/";
        }
        key += path[i];
    }
    recordSetting(report, key, value);
}

GraphicPackSelectionReport enableBestPack(
    CemuSettingsXml& settings,
    const std::filesystem::path& cemuRoot,
    GameId game,
    std::optional<std::uint64_t> titleId,
    std::string label,
    bool important,
    std::initializer_list<std::string_view> required,
    std::initializer_list<std::string_view> optional = {}) {
    GraphicPackSelectionReport report;
    report.label = std::move(label);
    report.important = important;
    const auto gameTerms = gameNeedles(game);
    const auto match = findBestGraphicPack(
        cemuRoot,
        std::span<const std::string_view>(gameTerms.data(), gameTerms.size()),
        std::span<const std::string_view>(required.begin(), required.size()),
        std::span<const std::string_view>(optional.begin(), optional.size()),
        titleId);
    if (match) {
        settings.enableGraphicPack(match->configPath);
        report.selected = true;
        report.packName = match->definitionName.empty() ? match->folderName : match->definitionName;
        report.configPath = match->configPath.generic_string();
    } else {
        report.warning = report.label + " graphic pack was not found.";
    }
    return report;
}

GraphicPackSelectionReport enableBestPresetPack(
    CemuSettingsXml& settings,
    const std::filesystem::path& cemuRoot,
    GameId game,
    std::optional<std::uint64_t> titleId,
    std::string label,
    bool important,
    const std::string& category,
    const std::string& fallbackPreset,
    std::initializer_list<std::string_view> packRequired,
    std::initializer_list<std::string_view> presetRequired,
    std::initializer_list<std::string_view> optional = {}) {
    GraphicPackSelectionReport report;
    report.label = std::move(label);
    report.important = important;
    report.presetCategory = category;
    const auto gameTerms = gameNeedles(game);
    const auto match = findBestGraphicPack(
        cemuRoot,
        std::span<const std::string_view>(gameTerms.data(), gameTerms.size()),
        std::span<const std::string_view>(packRequired.begin(), packRequired.size()),
        std::span<const std::string_view>(optional.begin(), optional.size()),
        titleId);
    if (!match) {
        report.warning = report.label + " graphic pack was not found.";
        return report;
    }

    settings.enableGraphicPack(match->configPath);
    const auto preset = findBestPresetSelection(
        *match,
        std::span<const std::string_view>(presetRequired.begin(), presetRequired.size()),
        std::span<const std::string_view>(optional.begin(), optional.size()));
    report.selected = true;
    report.packName = match->definitionName.empty() ? match->folderName : match->definitionName;
    report.configPath = match->configPath.generic_string();
    report.presetCategory = preset && !preset->category.empty() ? preset->category : category;
    report.preset = preset ? preset->name : fallbackPreset;
    settings.setGraphicPackPreset(match->configPath, report.presetCategory, report.preset);
    if (!preset) {
        report.warning = report.label + " preset was not found; using " + fallbackPreset + ".";
    }
    return report;
}

void addGraphicPackReport(CemuPreparationReport& report, GraphicPackSelectionReport selection) {
    if (!selection.selected && selection.important) {
        report.warnings.push_back(selection.warning);
    } else if (selection.selected && !selection.warning.empty()) {
        report.warnings.push_back(selection.warning);
    }
    report.graphicPacks.push_back(std::move(selection));
}

} // namespace

ProcessManager::ProcessManager(ProcessManagerConfig config)
    : config_(std::move(config)) {}

void ProcessManager::validateKeysFile(const std::filesystem::path& path) {
    validateKeysFileFormat(path);
}

GameLaunchSpec ProcessManager::resolveFromStub(const std::filesystem::path& stubPath) const {
    for (auto spec : config_.games) {
        const auto* entry = findGameCatalogEntry(spec.game);
        if (!entry) {
            continue;
        }
        const auto candidate = findGameCatalogEntryByStub(stubPath);
        if (candidate && candidate->game == entry->game) {
            spec.stubPath = stubPath;
            return spec;
        }
    }

    if (const auto* entry = findGameCatalogEntryByStub(stubPath)) {
        GameLaunchSpec spec;
        spec.game = entry->game;
        spec.displayName = entry->displayName;
        spec.stubPath = stubPath;
        return spec;
    }

    throw std::runtime_error("unrecognized Skylanders stub: " + stubPath.string());
}

RunningProcess ProcessManager::launchFromStub(const std::filesystem::path& stubPath) {
    return launch(resolveFromStub(stubPath));
}

RunningProcess ProcessManager::launch(const GameLaunchSpec& spec) {
    if (spec.game == GameId::Unknown) {
        throw std::invalid_argument("launch spec has no game id");
    }
    if (spec.gamePath.empty()) {
        throw std::invalid_argument("launch spec is missing the Wii U game path");
    }
    if (!spec.titleId) {
        throw std::invalid_argument("launch spec is missing the Wii U title id");
    }

    validateCemuInstall();
    prepareForLaunchDiagnostics(spec);
    return spawnCemu(spec);
}

CemuPreparationReport ProcessManager::prepareForLaunchDiagnostics(const GameLaunchSpec& spec) {
    if (spec.game == GameId::Unknown) {
        throw std::invalid_argument("launch spec has no game id");
    }
    if (spec.gamePath.empty()) {
        throw std::invalid_argument("launch spec is missing the Wii U game path");
    }
    if (!spec.titleId) {
        throw std::invalid_argument("launch spec is missing the Wii U title id");
    }
    validateCemuInstall();
    lastPreparationReport_ = prepareCemuEnvironment(spec);
    persistPreparationReport(lastPreparationReport_);
    return lastPreparationReport_;
}

CemuPreparationReport ProcessManager::prepareForEmbeddedDiagnostics(const GameLaunchSpec& spec) {
    if (spec.game == GameId::Unknown) {
        throw std::invalid_argument("launch spec has no game id");
    }
    if (spec.gamePath.empty()) {
        throw std::invalid_argument("launch spec is missing the Wii U game path");
    }
    if (!spec.titleId) {
        throw std::invalid_argument("launch spec is missing the Wii U title id");
    }
    validateCemuLayout();
    lastPreparationReport_ = prepareCemuEnvironment(spec);
    persistPreparationReport(lastPreparationReport_);
    return lastPreparationReport_;
}

const CemuPreparationReport& ProcessManager::lastPreparationReport() const {
    return lastPreparationReport_;
}

void ProcessManager::validateCemuInstall() const {
    const auto cemuExe = config_.cemuRoot / L"Cemu.exe";
    if (!std::filesystem::exists(cemuExe)) {
        throw std::runtime_error("Cemu.exe was not found at " + cemuExe.string());
    }
}

void ProcessManager::validateCemuLayout() const {
    if (config_.cemuRoot.empty()) {
        throw std::runtime_error("Cemu root is not configured.");
    }
    const auto resources = config_.cemuRoot / L"resources";
    if (!std::filesystem::exists(resources)) {
        throw std::runtime_error("Cemu resources folder was not found at " + resources.string());
    }
    const auto gameProfiles = config_.cemuRoot / L"gameProfiles";
    if (!std::filesystem::exists(gameProfiles)) {
        throw std::runtime_error("Cemu gameProfiles folder was not found at " + gameProfiles.string());
    }
}

CemuPreparationReport ProcessManager::prepareCemuEnvironment(const GameLaunchSpec& spec) const {
    CemuPreparationReport report;
    report.game = spec.game;
    report.settingsPath = cemuSettingsPath(config_);
    if (spec.titleId) {
        report.gameProfilePath = gameProfilePath(config_, *spec.titleId);
    }
    ensureRuntimeLayout(spec);
    installKeysFile();
    applyGlobalCemuSettings(spec, report);
    isolateControllerProfiles(spec, report);
    applyGameProfileOverrides(spec, report);
    return report;
}

void ProcessManager::installKeysFile() const {
    const auto source = config_.keysPath.empty() ? config_.runtimeRoot.parent_path() / L"keys" / L"keys.txt" : config_.keysPath;
    validateKeysFile(source);

    const auto target = config_.cemuRoot / L"keys.txt";
    std::filesystem::create_directories(target.parent_path());
    std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing);
}

void ProcessManager::ensureRuntimeLayout(const GameLaunchSpec& spec) const {
    const auto root = gameRuntimeRoot(config_, spec.game);
    for (const auto& dir : {
        root,
        root / L"controllerProfiles",
        root / L"nfc",
        root / L"figures",
        root / L"mlc01",
        root / L"mlc01\\sys",
        root / L"mlc01\\usr",
    }) {
        std::filesystem::create_directories(dir);
    }
}

void ProcessManager::applyGlobalCemuSettings(const GameLaunchSpec& spec, CemuPreparationReport& report) const {
    CemuSettingsXml settings(cemuSettingsPath(config_));
    settings.loadOrCreate();

    setAndRecord(settings, report, {"content", "fullscreen"}, config_.fullscreen ? "true" : "false");
    setAndRecord(settings, report, {"content", "fullscreen_menubar"}, "false");
    setAndRecord(settings, report, {"content", "gp_download"}, "true");
    setAndRecord(settings, report, {"content", "mlc_path"}, gameMlcPath(config_, spec.game).string());

    // Cemu upstream enum: 0 = OpenGL, 1 = Vulkan, 2 = Metal.
    setAndRecord(settings, report, {"content", "Graphic", "api"}, "1");
    setAndRecord(settings, report, {"content", "Graphic", "AsyncCompile"}, "true");
    setAndRecord(settings, report, {"content", "Graphic", "GX2DrawdoneSync"}, "true");
    setAndRecord(settings, report, {"content", "Graphic", "vkAccurateBarriers"}, "true");

    if (!settings.contentValue({"content", "Audio", "api"})) {
        setAndRecord(settings, report, {"content", "Audio", "api"}, "2");
    }
    if (!settings.contentValue({"content", "Audio", "delay"})) {
        setAndRecord(settings, report, {"content", "Audio", "delay"}, "2");
    }
    if (!settings.contentValue({"content", "Audio", "TVDevice"})) {
        setAndRecord(settings, report, {"content", "Audio", "TVDevice"}, "default");
    }

    setAndRecord(settings, report, {"content", "EmulatedUsbDevices", "EmulateSkylanderPortal"}, "true");

    const auto resolution = resolutionPresetName(chooseResolutionPreset());
    switch (spec.game) {
    case GameId::SpyrosAdventure:
        addGraphicPackReport(report, enableBestPresetPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Resolution", true, "Resolution", resolution, {"resolution"}, {resolution}, {"2k", "4k"}));
        break;
    case GameId::Giants:
        addGraphicPackReport(report, enableBestPresetPack(settings, config_.cemuRoot, spec.game, spec.titleId, "FPS", false, "FPS", "120FPS", {"fps"}, {"120"}, {"120", "fps"}));
        addGraphicPackReport(report, enableBestPresetPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Resolution", true, "Resolution", resolution, {"resolution"}, {resolution}, {"2k", "4k"}));
        addGraphicPackReport(report, enableBestPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Vulkan fix", false, {"vulkan", "fix"}, {"shader", "graphics"}));
        break;
    case GameId::SwapForce:
        addGraphicPackReport(report, enableBestPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Portal fix", true, {"portal", "fix"}, {"stability", "disconnect"}));
        addGraphicPackReport(report, enableBestPresetPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Resolution", true, "Resolution", resolution, {"resolution"}, {resolution}, {"2k", "4k"}));
        addGraphicPackReport(report, enableBestPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Vulkan fix", false, {"vulkan", "fix"}, {"shader", "graphics"}));
        break;
    case GameId::TrapTeam:
        addGraphicPackReport(report, enableBestPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Portal/traptanium fix", true, {"portal"}, {"traptanium", "compatibility", "fix"}));
        addGraphicPackReport(report, enableBestPresetPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Resolution", true, "Resolution", resolution, {"resolution"}, {resolution}, {"2k", "4k"}));
        addGraphicPackReport(report, enableBestPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Vulkan fix", false, {"vulkan", "fix"}, {"shader", "graphics"}));
        break;
    case GameId::SuperChargers:
    case GameId::Imaginators:
        addGraphicPackReport(report, enableBestPresetPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Resolution", true, "Resolution", resolution, {"resolution"}, {resolution}, {"2k", "4k"}));
        addGraphicPackReport(report, enableBestPack(settings, config_.cemuRoot, spec.game, spec.titleId, "Vulkan fix", false, {"vulkan", "fix"}, {"shader", "graphics"}));
        break;
    case GameId::Unknown:
        break;
    }

    settings.save();
}

void ProcessManager::applyGameProfileOverrides(const GameLaunchSpec& spec, CemuPreparationReport& report) const {
    if (!spec.titleId) {
        return;
    }

    IniDocument profile(gameProfilePath(config_, *spec.titleId));
    profile.loadIfExists();

    if (spec.game == GameId::Giants) {
        profile.set("Graphics", "graphics_api", "1");
    }

    profile.set("Controller", "controller1", "skylanders_player1");
    if (std::filesystem::exists(sharedPlayerProfilePath(config_, 2)) ||
        std::filesystem::exists(gamePlayerProfilePath(config_, spec.game, 2))) {
        profile.set("Controller", "controller2", "skylanders_player2");
    } else {
        profile.erase("Controller", "controller2");
    }
    for (int i = 3; i <= 8; ++i) {
        profile.erase("Controller", "controller" + std::to_string(i));
    }

    profile.save();
    report.gameProfilePath = gameProfilePath(config_, *spec.titleId);
}

void ProcessManager::isolateControllerProfiles(const GameLaunchSpec& spec, CemuPreparationReport& report) const {
    const auto activeProfiles = cemuControllerProfilesPath(config_);
    const auto gameProfiles = gameRuntimeRoot(config_, spec.game) / L"controllerProfiles";
    const auto sharedProfiles = sharedControllerProfilesPath(config_);
    std::filesystem::create_directories(activeProfiles);
    std::filesystem::create_directories(gameProfiles);
    std::filesystem::create_directories(sharedProfiles);

    const auto sharedPlayer1 = sharedPlayerProfilePath(config_, 1);
    if (!std::filesystem::exists(sharedPlayer1)) {
        copyIfPresent(gameProfiles / L"skylanders_player1.xml", sharedPlayer1);
    }
    if (!std::filesystem::exists(sharedPlayer1)) {
        copyIfPresent(activeProfiles / L"skylanders_player1.xml", sharedPlayer1);
    }
    if (!std::filesystem::exists(sharedPlayer1)) {
        copyIfPresent(activeProfiles / L"controller0.xml", sharedPlayer1);
    }
    if (!std::filesystem::exists(sharedPlayer1)) {
        writeDefaultKeyboardProProfile(sharedPlayer1, "skylanders_player1");
    }

    const auto backupRoot = gameRuntimeRoot(config_, spec.game) / L"controllerProfiles.backup";
    std::filesystem::create_directories(backupRoot);
    for (const auto& entry : std::filesystem::directory_iterator(activeProfiles)) {
        if (entry.is_regular_file()) {
            std::filesystem::copy_file(
                entry.path(),
                backupRoot / entry.path().filename(),
                std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove(entry.path());
        }
    }

    copyIfPresent(sharedPlayer1, gameProfiles / L"skylanders_player1.xml");
    copyIfPresent(sharedPlayer1, activeProfiles / L"skylanders_player1.xml");
    report.player1ProfilePath = sharedPlayer1;

    const auto sharedPlayer2 = sharedPlayerProfilePath(config_, 2);
    if (std::filesystem::exists(sharedPlayer2)) {
        copyIfPresent(sharedPlayer2, gameProfiles / L"skylanders_player2.xml");
        copyIfPresent(sharedPlayer2, activeProfiles / L"skylanders_player2.xml");
        report.player2ProfilePath = sharedPlayer2;
    }

    for (int i = 0; i < 8; ++i) {
        const auto filename = std::wstring(L"controller") + std::to_wstring(i) + L".xml";
        std::filesystem::remove(activeProfiles / filename);
    }
}

ResolutionPreset ProcessManager::chooseResolutionPreset() const {
    // Conservative default until the GPU capability probe is wired to DXGI.
    // The call site already routes this through Cemu graphic-pack presets.
    return ResolutionPreset::TwoK;
}

std::string ProcessManager::preparationReportJson(const CemuPreparationReport& report) {
    std::ostringstream json;
    json << "{";
    json << "\"game\":\"" << jsonEscape(gameIdToConfigKey(report.game)) << "\",";
    json << "\"settingsPath\":\"" << jsonEscape(report.settingsPath.string()) << "\",";
    json << "\"gameProfilePath\":\"" << jsonEscape(report.gameProfilePath.string()) << "\",";
    json << "\"player1ProfilePath\":\"" << jsonEscape(report.player1ProfilePath.string()) << "\",";
    json << "\"player2ProfilePath\":\"" << jsonEscape(report.player2ProfilePath.string()) << "\",";

    json << "\"appliedSettings\":[";
    for (std::size_t i = 0; i < report.appliedSettings.size(); ++i) {
        const auto& setting = report.appliedSettings[i];
        if (i != 0) {
            json << ",";
        }
        json << "{\"key\":\"" << jsonEscape(setting.key) << "\",\"value\":\"" << jsonEscape(setting.value) << "\"}";
    }
    json << "],";

    json << "\"selectedGraphicPacks\":[";
    bool wroteSelected = false;
    for (const auto& pack : report.graphicPacks) {
        if (!pack.selected) {
            continue;
        }
        if (wroteSelected) {
            json << ",";
        }
        wroteSelected = true;
        json << "{\"label\":\"" << jsonEscape(pack.label) << "\",";
        json << "\"packName\":\"" << jsonEscape(pack.packName) << "\",";
        json << "\"configPath\":\"" << jsonEscape(pack.configPath) << "\",";
        json << "\"presetCategory\":\"" << jsonEscape(pack.presetCategory) << "\",";
        json << "\"preset\":\"" << jsonEscape(pack.preset) << "\"}";
    }
    json << "],";

    json << "\"graphicPacks\":[";
    for (std::size_t i = 0; i < report.graphicPacks.size(); ++i) {
        const auto& pack = report.graphicPacks[i];
        if (i != 0) {
            json << ",";
        }
        json << "{\"label\":\"" << jsonEscape(pack.label) << "\",";
        json << "\"selected\":" << (pack.selected ? "true" : "false") << ",";
        json << "\"important\":" << (pack.important ? "true" : "false") << ",";
        json << "\"packName\":\"" << jsonEscape(pack.packName) << "\",";
        json << "\"configPath\":\"" << jsonEscape(pack.configPath) << "\",";
        json << "\"presetCategory\":\"" << jsonEscape(pack.presetCategory) << "\",";
        json << "\"preset\":\"" << jsonEscape(pack.preset) << "\",";
        json << "\"warning\":\"" << jsonEscape(pack.warning) << "\"}";
    }
    json << "],";

    json << "\"warnings\":[";
    for (std::size_t i = 0; i < report.warnings.size(); ++i) {
        if (i != 0) {
            json << ",";
        }
        json << "\"" << jsonEscape(report.warnings[i]) << "\"";
    }
    json << "]";
    json << "}";
    return json.str();
}

void ProcessManager::persistPreparationReport(const CemuPreparationReport& report) const {
    const auto path = preparationReportPath(config_);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (out) {
        out << preparationReportJson(report);
    }
}

RunningProcess ProcessManager::spawnCemu(const GameLaunchSpec& spec) const {
    const auto cemuExe = config_.cemuRoot / L"Cemu.exe";
    std::wstring commandLine = quoteArg(cemuExe);
    commandLine += L" -g ";
    commandLine += quoteArg(spec.gamePath);
    if (config_.fullscreen) {
        commandLine += L" -f";
    }
    commandLine += L" -m ";
    commandLine += quoteArg(gameMlcPath(config_, spec.game));

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};

    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    const BOOL ok = CreateProcessW(
        cemuExe.c_str(),
        mutableCommand.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_PROCESS_GROUP,
        nullptr,
        config_.cemuRoot.c_str(),
        &startup,
        &process);

    if (!ok) {
        throw std::runtime_error("CreateProcessW failed for Cemu.exe");
    }

    CloseHandle(process.hThread);
    return RunningProcess{process.dwProcessId, process.hProcess};
}

} // namespace skylanders
