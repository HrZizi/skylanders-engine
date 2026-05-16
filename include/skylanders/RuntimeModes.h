#pragma once

#include <string>
#include <optional>

namespace skylanders {

enum class CemuBackendMode {
    BundledExternal,
    EmbeddedNative,
    ExternalPicked,
};

enum class OverlayBackendMode {
    ExternalWindow,
    NativeRenderer,
};

std::string cemuBackendModeToString(CemuBackendMode mode);
std::string overlayBackendModeToString(OverlayBackendMode mode);
CemuBackendMode cemuBackendModeFromString(const std::string& value);
OverlayBackendMode overlayBackendModeFromString(const std::string& value);

struct CemuBackendSelection {
    CemuBackendMode requested = CemuBackendMode::BundledExternal;
    CemuBackendMode effective = CemuBackendMode::BundledExternal;
    bool explicitSelection = false;
    bool fallbackAllowed = true;
    bool fallbackUsed = false;
    std::string fallbackReason;
};

CemuBackendSelection selectCemuBackend(
    const std::optional<CemuBackendMode>& configuredMode,
    bool embeddedAvailable,
    bool externalAvailable);

} // namespace skylanders
