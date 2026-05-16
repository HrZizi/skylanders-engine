#include "skylanders/RuntimeModes.h"

namespace skylanders {

std::string cemuBackendModeToString(CemuBackendMode mode) {
    switch (mode) {
    case CemuBackendMode::BundledExternal: return "bundled_external";
    case CemuBackendMode::EmbeddedNative: return "embedded_native";
    case CemuBackendMode::ExternalPicked: return "external_picked";
    }
    return "bundled_external";
}

std::string overlayBackendModeToString(OverlayBackendMode mode) {
    switch (mode) {
    case OverlayBackendMode::ExternalWindow: return "external_window";
    case OverlayBackendMode::NativeRenderer: return "native_renderer";
    }
    return "external_window";
}

CemuBackendMode cemuBackendModeFromString(const std::string& value) {
    if (value == "embedded_native") {
        return CemuBackendMode::EmbeddedNative;
    }
    if (value == "external_picked") {
        return CemuBackendMode::ExternalPicked;
    }
    return CemuBackendMode::BundledExternal;
}

OverlayBackendMode overlayBackendModeFromString(const std::string& value) {
    if (value == "native_renderer") {
        return OverlayBackendMode::NativeRenderer;
    }
    return OverlayBackendMode::ExternalWindow;
}

CemuBackendSelection selectCemuBackend(
    const std::optional<CemuBackendMode>& configuredMode,
    bool embeddedAvailable,
    bool externalAvailable) {
    CemuBackendSelection selection;
    selection.explicitSelection = configuredMode.has_value();
    selection.fallbackAllowed = !selection.explicitSelection;
    selection.requested = configuredMode.value_or(CemuBackendMode::BundledExternal);
    selection.effective = selection.requested;

    if (selection.requested == CemuBackendMode::EmbeddedNative && !embeddedAvailable) {
        if (selection.fallbackAllowed && externalAvailable) {
            selection.effective = CemuBackendMode::BundledExternal;
            selection.fallbackUsed = true;
            selection.fallbackReason = "Embedded Cemu is unavailable in this build.";
        } else {
            selection.fallbackReason = "Embedded Cemu is unavailable in this build.";
        }
    }

    if (selection.requested != CemuBackendMode::EmbeddedNative && !externalAvailable) {
        selection.fallbackReason = "External Cemu is not configured.";
    }

    return selection;
}

} // namespace skylanders
