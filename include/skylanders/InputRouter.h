#pragma once

#include "skylanders/RuntimeModes.h"

#include <string>

namespace skylanders {

struct InputRouterState {
    bool overlayVisible = false;
    bool gameplayInputSuppressed = false;
    std::string lastError;
};

class InputRouter {
public:
    explicit InputRouter(OverlayBackendMode overlayMode = OverlayBackendMode::ExternalWindow);

    void setOverlayBackendMode(OverlayBackendMode mode);
    void setOverlayVisible(bool visible);
    bool shouldRouteToOverlay() const;
    bool shouldSuppressGameplayInput() const;
    InputRouterState state() const;

private:
    OverlayBackendMode overlayMode_;
    InputRouterState state_;
};

} // namespace skylanders
