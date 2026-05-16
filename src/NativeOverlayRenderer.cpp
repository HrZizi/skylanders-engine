#include "skylanders/NativeOverlayRenderer.h"

#include <sstream>

namespace skylanders {
namespace {

std::string escapeJson(std::string value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

} // namespace

bool NativeOverlayRenderer::initialize(std::uint32_t width, std::uint32_t height) {
    status_.available = false;
    status_.initialized = false;
    status_.width = width;
    status_.height = height;
    status_.lastError = "Native renderer overlay is planned for V2.3 and is not implemented in V1.6.";
    return false;
}

void NativeOverlayRenderer::resize(std::uint32_t width, std::uint32_t height) {
    status_.width = width;
    status_.height = height;
}

void NativeOverlayRenderer::update() {
}

bool NativeOverlayRenderer::renderToTexture() {
    status_.lastError = "Native renderer overlay is not available yet.";
    return false;
}

void NativeOverlayRenderer::setVisible(bool visible) {
    status_.visible = visible;
}

bool NativeOverlayRenderer::fireInputEvent(const std::string& eventJson) {
    (void)eventJson;
    status_.lastError = "Native renderer overlay input is not available yet.";
    return false;
}

std::string NativeOverlayRenderer::nativeStateJson() const {
    std::ostringstream json;
    json << "{";
    json << "\"available\":" << (status_.available ? "true" : "false") << ",";
    json << "\"initialized\":" << (status_.initialized ? "true" : "false") << ",";
    json << "\"visible\":" << (status_.visible ? "true" : "false") << ",";
    json << "\"width\":" << status_.width << ",";
    json << "\"height\":" << status_.height << ",";
    json << "\"lastError\":\"" << escapeJson(status_.lastError) << "\"";
    json << "}";
    return json.str();
}

NativeOverlayStatus NativeOverlayRenderer::status() const {
    return status_;
}

} // namespace skylanders
