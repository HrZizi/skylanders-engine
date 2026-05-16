#pragma once

#include "skylanders/InputBridge.h"
#include "skylanders/PortalBridge.h"
#include "skylanders/ProcessManager.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace skylanders {

struct EmbeddedCemuStatus {
    bool available = false;
    bool initialized = false;
    bool running = false;
    std::string lastError;
};

class EmbeddedCemuHost {
public:
    EmbeddedCemuHost();
    ~EmbeddedCemuHost();

    EmbeddedCemuHost(const EmbeddedCemuHost&) = delete;
    EmbeddedCemuHost& operator=(const EmbeddedCemuHost&) = delete;

    bool initialize();
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
    EmbeddedCemuStatus status_;
};

} // namespace skylanders
