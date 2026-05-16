#pragma once

#include <cstdint>
#include <string>

namespace skylanders {

struct NativeOverlayStatus {
    bool available = false;
    bool initialized = false;
    bool visible = false;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::string lastError;
};

class NativeOverlayRenderer {
public:
    bool initialize(std::uint32_t width, std::uint32_t height);
    void resize(std::uint32_t width, std::uint32_t height);
    void update();
    bool renderToTexture();
    void setVisible(bool visible);
    bool fireInputEvent(const std::string& eventJson);
    std::string nativeStateJson() const;
    NativeOverlayStatus status() const;

private:
    NativeOverlayStatus status_;
};

} // namespace skylanders
