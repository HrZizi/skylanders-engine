#include "skylanders/InputRouter.h"

namespace skylanders {

InputRouter::InputRouter(OverlayBackendMode overlayMode)
    : overlayMode_(overlayMode) {
}

void InputRouter::setOverlayBackendMode(OverlayBackendMode mode) {
    overlayMode_ = mode;
}

void InputRouter::setOverlayVisible(bool visible) {
    state_.overlayVisible = visible;
    state_.gameplayInputSuppressed = visible && overlayMode_ == OverlayBackendMode::NativeRenderer;
}

bool InputRouter::shouldRouteToOverlay() const {
    return state_.overlayVisible;
}

bool InputRouter::shouldSuppressGameplayInput() const {
    return state_.gameplayInputSuppressed;
}

InputRouterState InputRouter::state() const {
    return state_;
}

} // namespace skylanders
