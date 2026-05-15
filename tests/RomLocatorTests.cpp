#include "skylanders/CharacterCatalog.h"
#include "skylanders/CemuLocator.h"
#include "skylanders/FigureStore.h"
#include "skylanders/GameCatalog.h"
#include "skylanders/GraphicPackManager.h"
#include "skylanders/InputBridge.h"
#include "skylanders/PortalBridge.h"
#include "skylanders/ProcessManager.h"
#include "skylanders/RomLocator.h"

#include <cassert>
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace {

void writeText(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out << text;
}

void writeFigure(const std::filesystem::path& path, std::uint16_t skyId, std::uint16_t skyVar) {
    std::filesystem::create_directories(path.parent_path());
    std::array<unsigned char, 0x400> bytes{};
    bytes[0x10] = static_cast<unsigned char>(skyId & 0xFF);
    bytes[0x11] = static_cast<unsigned char>((skyId >> 8) & 0xFF);
    bytes[0x1C] = static_cast<unsigned char>(skyVar & 0xFF);
    bytes[0x1D] = static_cast<unsigned char>((skyVar >> 8) & 0xFF);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::filesystem::path makeExtractedTitle(const std::filesystem::path& root, const std::string& titleId) {
    std::filesystem::create_directories(root / "content");
    std::filesystem::create_directories(root / "meta");
    writeText(
        root / "code" / "app.xml",
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<app>\n"
        "  <title_id>" + titleId + "</title_id>\n"
        "</app>\n");
    writeText(root / "code" / "game.rpx", "not a real rpx; validation uses app.xml for extracted titles");
    return root;
}

void assertCatalogContainsAllTitleIds() {
    std::set<std::uint64_t> ids;
    for (const auto& entry : skylanders::gameCatalog()) {
        for (const auto& title : entry.acceptedBaseTitleIds) {
            ids.insert(title.value);
        }
    }

    assert(ids.contains(0x0005000010142D00ull));
    assert(ids.contains(0x000500001010D700ull));
    assert(ids.contains(0x0005000010116000ull));
    assert(ids.contains(0x0005000010139200ull));
    assert(ids.contains(0x0005000010140400ull));
    assert(ids.contains(0x000500001017C600ull));
    assert(ids.contains(0x0005000010181F00ull));
    assert(ids.contains(0x00050000101B8500ull));
    assert(ids.contains(0x00050000101BFC00ull));
    assert(ids.contains(0x00050000101F4D00ull));
    assert(ids.contains(0x00050000101FB100ull));
}

void assertStubResolution() {
    assert(skylanders::findGameCatalogEntryByStub("SpyrosAdventure.exe")->game == skylanders::GameId::SpyrosAdventure);
    assert(skylanders::findGameCatalogEntryByStub("Giants.exe")->game == skylanders::GameId::Giants);
    assert(skylanders::findGameCatalogEntryByStub("SwapForce.exe")->game == skylanders::GameId::SwapForce);
    assert(skylanders::findGameCatalogEntryByStub("TrapTeam.exe")->game == skylanders::GameId::TrapTeam);
    assert(skylanders::findGameCatalogEntryByStub("SuperChargers.exe")->game == skylanders::GameId::SuperChargers);
    assert(skylanders::findGameCatalogEntryByStub("Imaginators.exe")->game == skylanders::GameId::Imaginators);
}

void assertRomLocatorPrecedenceAndValidation() {
    const auto root = std::filesystem::temp_directory_path() / "SkylandersEngineTests";
    std::filesystem::remove_all(root);

    const auto roms = root / "roms";
    const auto runtimeConfig = root / "runtime" / "config" / "game_paths.json";
    skylanders::RomLocator locator({roms, runtimeConfig});

    const auto bundledGiants = makeExtractedTitle(roms / "Giants", "000500001010D700");
    const auto externalGiants = makeExtractedTitle(root / "external" / "Giants", "0005000010116000");
    writeText(runtimeConfig, "{\n  \"Giants\": \"" + externalGiants.generic_string() + "\"\n}\n");

    const auto resolvedGiants = locator.resolve(skylanders::GameId::Giants);
    assert(resolvedGiants);
    assert(resolvedGiants->source == skylanders::GameImageSource::BundledRomsFolder);
    assert(resolvedGiants->titleId == 0x000500001010D700ull);

    const auto wrongTrapTeam = makeExtractedTitle(root / "wrong" / "TrapTeam", "000500001010D700");
    const auto rejected = locator.validateForGame(skylanders::GameId::TrapTeam, wrongTrapTeam, skylanders::GameImageSource::PersistedExternalPath);
    assert(!rejected.valid);

    writeText(runtimeConfig, "{\n  \"SuperChargers\": \"" + (root / "missing" / "game.rpx").generic_string() + "\"\n}\n");
    const auto missing = locator.resolve(skylanders::GameId::SuperChargers);
    assert(!missing);
    const auto persistedAfterMissing = std::filesystem::exists(runtimeConfig)
        ? std::filesystem::file_size(runtimeConfig)
        : 0;
    assert(persistedAfterMissing > 0);

    std::error_code cleanupError;
    std::filesystem::remove_all(root, cleanupError);
}

void assertKeysValidationAcceptsCemuStyleLines() {
    const auto root = std::filesystem::temp_directory_path() / "SkylandersEngineKeyTests";
    std::filesystem::remove_all(root);

    const auto keys = root / "keys.txt";
    writeText(
        keys,
        "# User supplied Cemu keys\n"
        "[system]\n"
        "common_key = 000102030405060708090a0b0c0d0e0f\n"
        "00112233445566778899aabbccddeeff = title key description\n"
        "aa bb cc dd ee ff 00 11 22 33 44 55 66 77 88 99\n");

    skylanders::ProcessManager::validateKeysFile(keys);
    std::error_code cleanupError;
    std::filesystem::remove_all(root, cleanupError);
}

void assertKeysValidationRejectsMeaningfulLinesWithoutKeys() {
    const auto root = std::filesystem::temp_directory_path() / "SkylandersEngineBadKeyTests";
    std::filesystem::remove_all(root);

    const auto keys = root / "keys.txt";
    writeText(
        keys,
        "# Comment-only lines are fine\n"
        "[system]\n"
        "common_key = not-a-key\n");

    bool rejected = false;
    try {
        skylanders::ProcessManager::validateKeysFile(keys);
    } catch (const std::exception&) {
        rejected = true;
    }
    assert(rejected);
    std::filesystem::remove_all(root);
}

void assertCemuValidationAndPrecedence() {
    const auto root = std::filesystem::temp_directory_path() / "SkylandersEngineCemuLocatorTests";
    std::filesystem::remove_all(root);

    const auto exePath = root / "build" / "Release" / "Giants.exe";
    const auto setupPath = root / "runtime" / "config" / "setup.json";
    const auto bundled = exePath.parent_path() / "cemu" / "Cemu.exe";
    const auto persisted = root / "external" / "Cemu.exe";
    const auto portable = root / "cemu" / "Cemu.exe";
    const auto legacy = root / "legacy" / "Cemu.exe";

    skylanders::CemuLocator locator({root, exePath, setupPath, legacy});
    assert(!locator.validateCemuExe(root / "missing" / "Cemu.exe").valid);
    writeText(root / "external" / "not-cemu.exe", "");
    assert(!locator.validateCemuExe(root / "external" / "not-cemu.exe").valid);

    writeText(persisted, "");
    assert(locator.validateCemuExe(persisted).valid);
    skylanders::SetupConfig setup;
    setup.externalCemuExePath = persisted;
    setup.save(setupPath);
    auto resolved = locator.resolve();
    assert(resolved);
    assert(resolved->source == skylanders::CemuSource::PersistedExternal);
    assert(resolved->cemuRoot == persisted.parent_path());

    writeText(bundled, "");
    resolved = locator.resolve();
    assert(resolved);
    assert(resolved->source == skylanders::CemuSource::BundledSubmodule);

    std::filesystem::remove(bundled);
    setup.externalCemuExePath = root / "external" / "moved" / "Cemu.exe";
    setup.save(setupPath);
    writeText(portable, "");
    resolved = locator.resolve();
    assert(resolved);
    assert(resolved->source == skylanders::CemuSource::PortableLocal);
    assert(!skylanders::SetupConfig::load(setupPath).externalCemuExePath);

    std::filesystem::remove(portable);
    writeText(setupPath, "{ this is not valid json");
    assert(!skylanders::SetupConfig::load(setupPath).externalCemuExePath);
    writeText(legacy, "");
    resolved = locator.resolve();
    assert(resolved);
    assert(resolved->source == skylanders::CemuSource::LegacyFallback);

    std::filesystem::remove_all(root);
}

void assertCharacterCatalogMappings() {
    assert(skylanders::characterCatalog().size() == 481);

    const auto* spyro = skylanders::findCharacter("spyro");
    assert(spyro);
    assert(spyro->skyId == 16);
    assert(spyro->skyVar == 0);
    assert(spyro->toyKind == skylanders::ToyKind::Skylander);
    assert(spyro->assetKey == "spyro");

    const auto* series2Spyro = skylanders::findCharacter("series_2_spyro");
    assert(series2Spyro);
    assert(series2Spyro->skyId == 16);
    assert(series2Spyro->skyVar == 0x1801);
    assert(series2Spyro->isVariant);
    assert(series2Spyro->releaseGame == skylanders::GameId::Giants);

    const auto* lightCoreWarnado = skylanders::findCharacter("lightcore_warnado");
    assert(lightCoreWarnado);
    assert(lightCoreWarnado->releaseGame == skylanders::GameId::SwapForce);

    const auto* eonsEliteSpyro = skylanders::findCharacter("eons_elite_spyro");
    assert(eonsEliteSpyro);
    assert(eonsEliteSpyro->releaseGame == skylanders::GameId::TrapTeam);

    const auto* eonsEliteDinoRang = skylanders::findCharacter("eons_elite_dino_rang");
    assert(eonsEliteDinoRang);
    assert(eonsEliteDinoRang->releaseGame == skylanders::GameId::SuperChargers);

    const auto* darkSpyro = skylanders::findCharacter("dark_spyro");
    assert(darkSpyro);
    assert(darkSpyro->skyId == 28);

    const auto* treeRex = skylanders::findCharacter("tree_rex");
    assert(treeRex);
    assert(treeRex->releaseGame == skylanders::GameId::Giants);
    assert(treeRex->skyId == 112);

    const auto* snapShot = skylanders::findCharacter("snap_shot");
    assert(snapShot);
    assert(snapShot->skyId == 462);
    assert(snapShot->skyVar == 0x3000);

    const auto* blastZoneTop = skylanders::findCharacter("blast_zone_top");
    assert(blastZoneTop);
    assert(blastZoneTop->skyId == 2004);
    assert(blastZoneTop->swapHalf == "top");
    assert(blastZoneTop->swapGroup == "blast_zone");

    const auto* blastZoneBottom = skylanders::findCharacter("blast_zone_bottom");
    assert(blastZoneBottom);
    assert(blastZoneBottom->skyId == 1004);
    assert(blastZoneBottom->swapHalf == "bottom");
    assert(blastZoneBottom->swapGroup == "blast_zone");

    const auto* kingPen = skylanders::findCharacter("king_pen");
    assert(kingPen);
    assert(kingPen->releaseGame == skylanders::GameId::Imaginators);

    const auto* kaosTrap = skylanders::findCharacter("kaos_trap");
    assert(kaosTrap);
    assert(kaosTrap->toyKind == skylanders::ToyKind::Trap);
    assert(kaosTrap->skyId == 220);

    const auto* handOfFate = skylanders::findCharacter("hand_of_fate");
    assert(handOfFate);
    assert(handOfFate->toyKind == skylanders::ToyKind::Item);

    const auto* hotStreak = skylanders::findCharacter("hot_streak");
    assert(hotStreak);
    assert(hotStreak->toyKind == skylanders::ToyKind::Vehicle);

    const auto* skyTrophy = skylanders::findCharacter("sky_trophy");
    assert(skyTrophy);
    assert(skyTrophy->toyKind == skylanders::ToyKind::Trophy);

    assert(skylanders::isExcludedVariantCategory("In-Game Variants"));
    assert(skylanders::isExcludedVariantCategory("Chase Variants"));
    assert(!skylanders::isExcludedVariantCategory("Trap Team Figures"));

    const auto json = skylanders::characterCatalogJson();
    assert(json.find("\"id\":\"spyro\"") != std::string::npos);
    assert(json.find("\"assetKey\":\"spyro\"") != std::string::npos);
    assert(json.find("\"toyKind\":\"Trap\"") != std::string::npos);
    assert(json.find("\"games\"") != std::string::npos);
    assert(json.find("\"swapHalf\":\"top\"") != std::string::npos);
}

void assertFigureValidation() {
    const auto root = std::filesystem::temp_directory_path() / "SkylandersEngineFigureTests";
    std::filesystem::remove_all(root);

    const auto validFigure = root / "spyro.sky";
    writeFigure(validFigure, 16, 0);
    auto validation = skylanders::validateFigureFile(validFigure, 16, 0);
    assert(validation.valid);
    assert(validation.exists);
    assert(validation.ids);
    assert(validation.ids->skyId == 16);

    validation = skylanders::validateFigureFile(validFigure, 112, 0);
    assert(!validation.valid);
    assert(validation.exists);
    assert(validation.reason.find("different character") != std::string::npos);

    validation = skylanders::validateFigureFile(root / "missing.sky", 16, 0);
    assert(!validation.valid);
    assert(!validation.exists);

    std::filesystem::remove_all(root);
}

void assertPortalBridgeMessages() {
    const auto message = skylanders::PortalBridge::formatPlaceOrCreateMessage(
        2,
        "E:/skylanders_port/figures/SpyrosAdventure/spyro.sky",
        16,
        0);
    assert(message.find("\"op\":\"place_or_create\"") != std::string::npos);
    assert(message.find("\"slot\":2") != std::string::npos);
    assert(message.find("\"skyId\":16") != std::string::npos);
    assert(message.find("\"skyVar\":0") != std::string::npos);

    const auto success = skylanders::PortalBridge::parseResponse(
        "{\"ok\":true,\"op\":\"place_or_create\",\"slot\":2,\"created\":true,\"portalSlot\":7}\n");
    assert(success.ok);
    assert(success.created);
    assert(success.portalSlot && *success.portalSlot == 7);

    const auto failure = skylanders::PortalBridge::parseResponse(
        "{\"ok\":false,\"error\":\"Figure file belongs to a different character\"}\n");
    assert(!failure.ok);
    assert(failure.error.find("different character") != std::string::npos);
}

void assertInputBridgeMessages() {
    const auto capture = skylanders::InputBridge::formatCaptureNextMessage("XInput", "0", "Controller", 18, 6000);
    assert(capture.find("\"op\":\"capture_next\"") != std::string::npos);
    assert(capture.find("\"api\":\"XInput\"") != std::string::npos);
    assert(capture.find("\"mapping\":18") != std::string::npos);

    const auto start = skylanders::InputBridge::formatStartCaptureMessage("XInput", "0", "Controller", 18, "axis");
    assert(start.find("\"op\":\"start_capture\"") != std::string::npos);
    assert(start.find("\"captureMode\":\"axis\"") != std::string::npos);

    const auto poll = skylanders::InputBridge::formatPollCaptureMessage(7);
    assert(poll.find("\"op\":\"poll_capture\"") != std::string::npos);
    assert(poll.find("\"captureId\":7") != std::string::npos);

    const auto cancel = skylanders::InputBridge::formatCancelCaptureMessage(7);
    assert(cancel.find("\"op\":\"cancel_capture\"") != std::string::npos);
    assert(cancel.find("\"captureId\":7") != std::string::npos);

    const auto read = skylanders::InputBridge::formatReadInputMessage("SDLController", "pad", "DualSense", 0);
    assert(read.find("\"op\":\"read_input\"") != std::string::npos);
    assert(read.find("\"sourceInputId\":0") != std::string::npos);

    const auto load = skylanders::InputBridge::formatLoadProfileMessage(1, "skylanders_player2");
    assert(load.find("\"op\":\"load_profile\"") != std::string::npos);
    assert(load.find("\"playerIndex\":1") != std::string::npos);
    assert(load.find("skylanders_player2") != std::string::npos);

    const auto success = skylanders::InputBridge::parseResponse(
        "{\"ok\":true,\"sourceInputId\":42,\"displayLabel\":\"X-Axis+\"}\n");
    assert(success.ok);
    assert(success.json.find("sourceInputId") != std::string::npos);

    const auto failure = skylanders::InputBridge::parseResponse(
        "{\"ok\":false,\"error\":\"Controller mapping requires the bundled Cemu build\"}\n");
    assert(!failure.ok);
    assert(failure.error.find("bundled Cemu") != std::string::npos);
}

void assertGraphicPackWebsiteParsing() {
    const std::string html =
        "<html><body>"
        "<a href=\"/downloads/readme.txt\">Readme</a>"
        "<a href=\"https://example.invalid/cemu_graphic_packs_v6.zip\">Graphic packs</a>"
        "</body></html>";
    const auto url = skylanders::GraphicPackManager::findVersion6ZipUrl(html, "https://cemu-project.github.io/cemu_graphic_packs/");
    assert(url);
    assert(*url == "https://example.invalid/cemu_graphic_packs_v6.zip");

    const auto relative = skylanders::GraphicPackManager::findVersion6ZipUrl(
        "<a href=\"packs/graphicPacks.zip\">zip</a>",
        "https://cemu-project.github.io/cemu_graphic_packs/");
    assert(relative);
    assert(*relative == "https://cemu-project.github.io/cemu_graphic_packs/packs/graphicPacks.zip");
}

void assertProcessPreparationDiagnostics() {
    const auto root = std::filesystem::current_path() / "build" / "test-tmp" / "SkylandersEnginePreparationTests";
    std::filesystem::remove_all(root);

    writeText(root / "keys" / "keys.txt", "common_key = 000102030405060708090a0b0c0d0e0f\n");
    writeText(root / "cemu" / "Cemu.exe", "");
    writeText(
        root / "cemu" / "graphicPacks" / "SkylandersGiantsResolution" / "rules.txt",
        "[Definition]\n"
        "titleIds = 000500001010D700\n"
        "name = Skylanders Giants Resolution\n"
        "path = Skylanders Giants/Graphics/Resolution\n"
        "\n"
        "[Preset]\n"
        "name = 2560x1440\n");
    writeText(
        root / "cemu" / "graphicPacks" / "SkylandersGiantsFPS" / "rules.txt",
        "[Definition]\n"
        "titleIds = 000500001010D700\n"
        "name = FPS\n"
        "path = Skylanders Giants/Mods/FPS\n"
        "\n"
        "[Preset]\n"
        "category = FPS Limit\n"
        "name = 120 FPS\n");

    skylanders::ProcessManagerConfig config;
    config.cemuRoot = root / "cemu";
    config.runtimeRoot = root / "runtime";
    config.keysPath = root / "keys" / "keys.txt";
    config.fullscreen = false;

    skylanders::GameLaunchSpec spec;
    spec.game = skylanders::GameId::Giants;
    spec.displayName = L"Giants";
    spec.gamePath = root / "roms" / "Giants";
    spec.titleId = 0x000500001010D700ull;

    skylanders::ProcessManager manager(config);
    manager.prepareForLaunchDiagnostics(spec);

    const auto report = manager.lastPreparationReport();
    assert(report.game == skylanders::GameId::Giants);
    assert(!report.graphicPacks.empty());
    assert(std::any_of(report.graphicPacks.begin(), report.graphicPacks.end(), [](const auto& pack) {
        return pack.label == "Resolution" && pack.selected && pack.preset == "2560x1440";
    }));
    assert(std::any_of(report.graphicPacks.begin(), report.graphicPacks.end(), [](const auto& pack) {
        return pack.label == "FPS" && pack.selected && pack.presetCategory == "FPS Limit" && pack.preset == "120 FPS";
    }));
    assert(std::any_of(report.graphicPacks.begin(), report.graphicPacks.end(), [](const auto& pack) {
        return pack.label == "Vulkan fix" && !pack.selected;
    }));

    const auto settings = std::filesystem::exists(root / "cemu" / "settings.xml")
        ? std::filesystem::file_size(root / "cemu" / "settings.xml")
        : 0;
    assert(settings > 0);
    const auto diagnostics = std::filesystem::exists(root / "runtime" / "config" / "last_launch_diagnostics.json")
        ? std::filesystem::file_size(root / "runtime" / "config" / "last_launch_diagnostics.json")
        : 0;
    assert(diagnostics > 0);
    const auto json = skylanders::ProcessManager::preparationReportJson(report);
    assert(json.find("\"selectedGraphicPacks\"") != std::string::npos);
    assert(json.find("Graphic packs") == std::string::npos);
    std::ifstream settingsXml(root / "cemu" / "settings.xml", std::ios::binary);
    std::ostringstream settingsText;
    settingsText << settingsXml.rdbuf();
    assert(settingsText.str().find("<category>Resolution</category>") != std::string::npos);
    assert(settingsText.str().find("<preset>2560x1440</preset>") != std::string::npos);
    assert(settingsText.str().find("<category>FPS Limit</category>") != std::string::npos);
    assert(settingsText.str().find("<preset>120 FPS</preset>") != std::string::npos);

    std::error_code cleanupError;
    std::filesystem::remove_all(root, cleanupError);
}

} // namespace

int main() {
    assertCatalogContainsAllTitleIds();
    assertStubResolution();
    assertCharacterCatalogMappings();
    assertFigureValidation();
    assertPortalBridgeMessages();
    assertInputBridgeMessages();
    assertGraphicPackWebsiteParsing();
    assertProcessPreparationDiagnostics();
    assertRomLocatorPrecedenceAndValidation();
    assertKeysValidationAcceptsCemuStyleLines();
    assertKeysValidationRejectsMeaningfulLinesWithoutKeys();
    assertCemuValidationAndPrecedence();
    return 0;
}
