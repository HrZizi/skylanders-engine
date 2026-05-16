#pragma once

#include <string>

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

} // namespace skylanders
