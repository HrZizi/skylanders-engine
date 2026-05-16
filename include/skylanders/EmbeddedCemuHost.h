#pragma once

#include "skylanders/InputBridge.h"
#include "skylanders/PortalBridge.h"
#include "skylanders/ProcessManager.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace skylanders {

enum class EmbeddedCemuPhase {
    Unavailable,
    Configured,
    Initialized,
    TitlePrepared,
    RunningUnsupported,
};

struct EmbeddedCemuConfig {
    std::filesystem::path cemuRoot;
    std::filesystem::path settingsPath;
    std::filesystem::path mlcPath;
    std::filesystem::path gameProfilePath;
    std::filesystem::path controllerProfileRoot;
    std::filesystem::path keysPath;
    std::filesystem::path graphicPacksRoot;
};

struct EmbeddedCemuStatus {
    bool available = false;
    bool configured = false;
    bool initialized = false;
    bool titlePrepared = false;
    bool running = false;
    bool fallbackUsed = false;
    EmbeddedCemuPhase phase = EmbeddedCemuPhase::Unavailable;
    std::string fallbackReason;
    std::string lastError;
};

std::string embeddedCemuPhaseToString(EmbeddedCemuPhase phase);

class EmbeddedCemuHost {
public:
    EmbeddedCemuHost();
    ~EmbeddedCemuHost();

    EmbeddedCemuHost(const EmbeddedCemuHost&) = delete;
    EmbeddedCemuHost& operator=(const EmbeddedCemuHost&) = delete;

    bool configure(const EmbeddedCemuConfig& config);
    bool initialize();
    bool prepareTitle(const GameLaunchSpec& spec);
    bool launch(const GameLaunchSpec& spec);
    void shutdown();
    EmbeddedCemuStatus status() const;

    PortalCommandResult placeOrCreate(
        int slot,
        const std::filesystem::path& figurePath,
        std::uint16_t skyId,
        std::uint16_t skyVar);
    PortalCommandResult remove(int slot);

    InputBridgeResult listInputDevices() const;
    InputBridgeResult captureInput(
        const std::string& api,
        const std::string& uuid,
        const std::string& displayName,
        int mapping,
        const std::string& captureMode) const;

private:
    std::optional<EmbeddedCemuConfig> config_;
    EmbeddedCemuStatus status_;
};

} // namespace skylanders
