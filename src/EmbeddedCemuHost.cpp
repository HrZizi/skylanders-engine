#include "skylanders/EmbeddedCemuHost.h"

#include "skylanders/FigureStore.h"

#if SKYLANDERS_HAS_CEMU
#include "Common/precompiled.h"
#include "Cafe/OS/libs/nsyshid/Skylander.h"
#include "Cafe/CafeSystem.h"
#include "Cafe/TitleList/TitleInfo.h"
#include "Cafe/TitleList/TitleList.h"
#include "Common/FileStream.h"
#include "SkylandersEngineBootstrap.h"
#endif

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace skylanders {
namespace {

std::vector<std::uint8_t> readFigureBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    std::vector<std::uint8_t> bytes(0x400);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (file.gcount() != static_cast<std::streamsize>(bytes.size())) {
        return {};
    }
    return bytes;
}

PortalCommandResult failedPortal(std::string error) {
    PortalCommandResult result;
    result.error = std::move(error);
    return result;
}

InputBridgeResult unavailableInput() {
    return {false, "{}", "Embedded Cemu input bridge is not implemented yet."};
}

} // namespace

std::string embeddedCemuPhaseToString(EmbeddedCemuPhase phase) {
    switch (phase) {
    case EmbeddedCemuPhase::Unavailable: return "unavailable";
    case EmbeddedCemuPhase::Configured: return "configured";
    case EmbeddedCemuPhase::Initialized: return "initialized";
    case EmbeddedCemuPhase::TitlePrepared: return "title_prepared";
    case EmbeddedCemuPhase::RunningUnsupported: return "running_unsupported";
    }
    return "unavailable";
}

EmbeddedCemuHost::EmbeddedCemuHost() {
#if SKYLANDERS_HAS_CEMU
    status_.available = true;
    status_.phase = EmbeddedCemuPhase::Unavailable;
#else
    status_.available = false;
    status_.phase = EmbeddedCemuPhase::Unavailable;
    status_.lastError = "SkylandersEngine was built without embedded Cemu support.";
#endif
}

EmbeddedCemuHost::~EmbeddedCemuHost() {
    shutdown();
}

bool EmbeddedCemuHost::configure(const EmbeddedCemuConfig& config) {
#if SKYLANDERS_HAS_CEMU
    std::error_code ec;
    if (config.cemuRoot.empty()) {
        status_.lastError = "Embedded Cemu root is not configured.";
        return false;
    }
    if (!std::filesystem::exists(config.cemuRoot / "resources", ec)) {
        status_.lastError = "Embedded Cemu resources folder is missing.";
        return false;
    }
    if (!std::filesystem::exists(config.cemuRoot / "gameProfiles", ec)) {
        status_.lastError = "Embedded Cemu gameProfiles folder is missing.";
        return false;
    }

    const SkylandersEngineBootstrapConfig bootstrapConfig{
        true,
        config.cemuRoot / "Cemu.exe",
        config.cemuRoot,
        config.cemuRoot,
        config.cemuRoot / "shaderCache",
        config.cemuRoot,
    };
    if (!SkylandersEngineBootstrapConfigure(bootstrapConfig)) {
        status_.lastError = "Cemu bootstrap paths could not be configured.";
        return false;
    }

    config_ = config;
    status_.configured = true;
    status_.phase = EmbeddedCemuPhase::Configured;
    status_.lastError.clear();
    return true;
#else
    (void)config;
    status_.lastError = "Embedded Cemu is unavailable in this build.";
    return false;
#endif
}

bool EmbeddedCemuHost::initialize() {
#if SKYLANDERS_HAS_CEMU
    if (!status_.configured) {
        status_.lastError = "Embedded Cemu has not been configured.";
        return false;
    }
    if (status_.initialized) {
        return true;
    }
    if (!SkylandersEngineBootstrapInitialize()) {
        status_.lastError = "Cemu bootstrap initialization failed.";
        return false;
    }
    status_.initialized = true;
    status_.phase = EmbeddedCemuPhase::Initialized;
    status_.lastError.clear();
    return true;
#else
    status_.lastError = "Embedded Cemu is unavailable in this build.";
    return false;
#endif
}

bool EmbeddedCemuHost::prepareTitle(const GameLaunchSpec& spec) {
#if SKYLANDERS_HAS_CEMU
    if (!initialize()) {
        return false;
    }
    if (spec.gamePath.empty()) {
        status_.lastError = "Launch spec is missing a game path.";
        return false;
    }

    TitleInfo launchTitle{ spec.gamePath };
    if (launchTitle.IsValid()) {
        CafeTitleList::AddTitleFromPath(spec.gamePath);
        TitleId baseTitleId{};
        if (!CafeTitleList::FindBaseTitleId(launchTitle.GetAppTitleId(), baseTitleId)) {
            status_.lastError = "Cemu could not find the base title id.";
            return false;
        }
        if (CafeSystem::PrepareForegroundTitle(baseTitleId) != CafeSystem::PREPARE_STATUS_CODE::SUCCESS) {
            status_.lastError = "Cemu could not prepare the foreground title.";
            return false;
        }
    } else {
        const CafeTitleFileType fileType = DetermineCafeSystemFileType(spec.gamePath);
        if (fileType != CafeTitleFileType::RPX && fileType != CafeTitleFileType::ELF) {
            status_.lastError = "Embedded Cemu can only launch valid titles or standalone RPX/ELF paths.";
            return false;
        }
        if (CafeSystem::PrepareForegroundTitleFromStandaloneRPX(spec.gamePath) != CafeSystem::PREPARE_STATUS_CODE::SUCCESS) {
            status_.lastError = "Cemu could not prepare the standalone RPX/ELF.";
            return false;
        }
    }

    status_.titlePrepared = true;
    status_.phase = EmbeddedCemuPhase::TitlePrepared;
    status_.lastError.clear();
    return true;
#else
    (void)spec;
    status_.lastError = "Embedded Cemu is unavailable in this build.";
    return false;
#endif
}

bool EmbeddedCemuHost::launch(const GameLaunchSpec& spec) {
#if SKYLANDERS_HAS_CEMU
    if (!prepareTitle(spec)) {
        return false;
    }
    status_.phase = EmbeddedCemuPhase::RunningUnsupported;
    status_.lastError = "Embedded render host is not implemented in V2.1.";
    return false;
#else
    (void)spec;
    status_.lastError = "Embedded Cemu is unavailable in this build.";
    return false;
#endif
}

void EmbeddedCemuHost::shutdown() {
#if SKYLANDERS_HAS_CEMU
    if (status_.running) {
        CafeSystem::ShutdownTitle();
    }
    if (status_.initialized) {
        SkylandersEngineBootstrapShutdown();
    }
#endif
    status_.running = false;
    status_.initialized = false;
    status_.titlePrepared = false;
    if (status_.configured) {
        status_.phase = EmbeddedCemuPhase::Configured;
    } else {
        status_.phase = EmbeddedCemuPhase::Unavailable;
    }
}

EmbeddedCemuStatus EmbeddedCemuHost::status() const {
    return status_;
}

PortalCommandResult EmbeddedCemuHost::placeOrCreate(
    int slot,
    const std::filesystem::path& figurePath,
    std::uint16_t skyId,
    std::uint16_t skyVar) {
#if SKYLANDERS_HAS_CEMU
    if (!status_.running) {
        return failedPortal("Embedded Cemu is not running.");
    }
    if (slot < 0) {
        return failedPortal("Invalid portal slot.");
    }
    std::error_code ec;
    bool created = false;
    if (!std::filesystem::exists(figurePath, ec)) {
        std::filesystem::create_directories(figurePath.parent_path(), ec);
        if (!nsyshid::g_skyportal.CreateSkylander(figurePath, skyId, skyVar)) {
            return failedPortal("Cemu could not create the Skylander figure file.");
        }
        created = true;
    }
    const auto validation = validateFigureFile(figurePath, skyId, skyVar);
    if (!validation.valid) {
        return failedPortal(validation.reason.empty() ? "Figure validation failed." : validation.reason);
    }
    const auto bytes = readFigureBytes(figurePath);
    if (bytes.empty()) {
        return failedPortal("Figure file is not a readable 0x400-byte Skylander dump.");
    }
    std::unique_ptr<FileStream> skyFile(FileStream::openFile2(figurePath, true));
    if (!skyFile) {
        return failedPortal("Unable to open Skylander figure file.");
    }
    std::array<uint8, nsyshid::SKY_FIGURE_SIZE> cemuBytes{};
    std::copy(bytes.begin(), bytes.end(), cemuBytes.begin());
    const uint8 portalSlot = nsyshid::g_skyportal.LoadSkylander(cemuBytes.data(), std::move(skyFile));
    if (portalSlot == 0xFF) {
        return failedPortal("Cemu could not load the Skylander onto the portal.");
    }
    PortalCommandResult result;
    result.ok = true;
    result.created = created;
    result.portalSlot = static_cast<int>(portalSlot);
    return result;
#else
    (void)slot;
    (void)figurePath;
    (void)skyId;
    (void)skyVar;
    return failedPortal("Embedded Cemu is unavailable in this build.");
#endif
}

PortalCommandResult EmbeddedCemuHost::remove(int slot) {
#if SKYLANDERS_HAS_CEMU
    if (!status_.running) {
        return failedPortal("Embedded Cemu is not running.");
    }
    if (slot < 0) {
        return failedPortal("Invalid portal slot.");
    }
    const bool removed = nsyshid::g_skyportal.RemoveSkylander(static_cast<uint8>(slot));
    if (!removed) {
        return failedPortal("Cemu could not remove the Skylander from the portal.");
    }
    PortalCommandResult result;
    result.ok = true;
    return result;
#else
    (void)slot;
    return failedPortal("Embedded Cemu is unavailable in this build.");
#endif
}

InputBridgeResult EmbeddedCemuHost::listInputDevices() const {
    return unavailableInput();
}

InputBridgeResult EmbeddedCemuHost::captureInput(
    const std::string& api,
    const std::string& uuid,
    const std::string& displayName,
    int mapping,
    const std::string& captureMode) const {
    (void)api;
    (void)uuid;
    (void)displayName;
    (void)mapping;
    (void)captureMode;
    return unavailableInput();
}

} // namespace skylanders
