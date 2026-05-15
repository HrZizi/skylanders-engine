#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "skylanders/ProcessManager.h"

namespace skylanders {

struct PortalCommandResult {
    bool ok = false;
    std::string error;
    bool created = false;
    std::optional<int> portalSlot;
};

class PortalBridge {
public:
    PortalCommandResult place(unsigned long cemuProcessId, int slot, const std::filesystem::path& figurePath) const;
    PortalCommandResult placeOrCreate(
        unsigned long cemuProcessId,
        int slot,
        const std::filesystem::path& figurePath,
        std::uint16_t skyId,
        std::uint16_t skyVar) const;
    PortalCommandResult remove(unsigned long cemuProcessId, int slot) const;
    bool ping(unsigned long cemuProcessId) const;

    static std::string formatPlaceMessage(int slot, const std::filesystem::path& figurePath);
    static std::string formatPlaceOrCreateMessage(
        int slot,
        const std::filesystem::path& figurePath,
        std::uint16_t skyId,
        std::uint16_t skyVar);
    static std::string formatRemoveMessage(int slot);
    static std::string formatPingMessage();
    static PortalCommandResult parseResponse(std::string_view jsonLine);
    static std::wstring pipeName(unsigned long cemuProcessId);

private:
    PortalCommandResult send(unsigned long cemuProcessId, const std::string& jsonLine) const;
};

} // namespace skylanders
