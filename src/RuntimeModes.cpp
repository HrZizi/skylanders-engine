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

} // namespace skylanders
