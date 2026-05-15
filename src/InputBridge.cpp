#include "skylanders/InputBridge.h"

#include <regex>
#include <sstream>
#include <string_view>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace skylanders {
namespace {

constexpr DWORD kPipeTimeoutMs = 250;

std::string escapeJson(std::string value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

std::string unescapeJson(std::string value) {
    std::string out;
    out.reserve(value.size());
    bool escaping = false;
    for (const char ch : value) {
        if (escaping) {
            switch (ch) {
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            default: out.push_back(ch); break;
            }
            escaping = false;
        } else if (ch == '\\') {
            escaping = true;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

InputBridgeResult failed(std::string error) {
    InputBridgeResult result;
    result.error = std::move(error);
    return result;
}

} // namespace

bool InputBridge::ping(unsigned long cemuProcessId) const {
    return send(cemuProcessId, formatPingMessage()).ok;
}

InputBridgeResult InputBridge::listDevices(unsigned long cemuProcessId) const {
    return send(cemuProcessId, formatListDevicesMessage());
}

InputBridgeResult InputBridge::captureNext(
    unsigned long cemuProcessId,
    const std::string& api,
    const std::string& uuid,
    const std::string& displayName,
    int mapping,
    int timeoutMs) const {
    return send(cemuProcessId, formatCaptureNextMessage(api, uuid, displayName, mapping, timeoutMs));
}

InputBridgeResult InputBridge::startCapture(
    unsigned long cemuProcessId,
    const std::string& api,
    const std::string& uuid,
    const std::string& displayName,
    int mapping,
    const std::string& captureMode) const {
    return send(cemuProcessId, formatStartCaptureMessage(api, uuid, displayName, mapping, captureMode));
}

InputBridgeResult InputBridge::pollCapture(unsigned long cemuProcessId, int captureId) const {
    return send(cemuProcessId, formatPollCaptureMessage(captureId));
}

InputBridgeResult InputBridge::cancelCapture(unsigned long cemuProcessId, int captureId) const {
    return send(cemuProcessId, formatCancelCaptureMessage(captureId));
}

InputBridgeResult InputBridge::readInput(
    unsigned long cemuProcessId,
    const std::string& api,
    const std::string& uuid,
    const std::string& displayName,
    unsigned long long sourceInputId) const {
    return send(cemuProcessId, formatReadInputMessage(api, uuid, displayName, sourceInputId));
}

InputBridgeResult InputBridge::loadProfile(unsigned long cemuProcessId, int playerIndex, const std::string& profileName) const {
    return send(cemuProcessId, formatLoadProfileMessage(playerIndex, profileName));
}

std::string InputBridge::formatPingMessage() {
    return "{\"op\":\"ping\"}\n";
}

std::string InputBridge::formatListDevicesMessage() {
    return "{\"op\":\"list_devices\"}\n";
}

std::string InputBridge::formatCaptureNextMessage(
    const std::string& api,
    const std::string& uuid,
    const std::string& displayName,
    int mapping,
    int timeoutMs) {
    std::ostringstream message;
    message << "{\"op\":\"capture_next\",\"api\":\"" << escapeJson(api)
            << "\",\"uuid\":\"" << escapeJson(uuid)
            << "\",\"displayName\":\"" << escapeJson(displayName)
            << "\",\"mapping\":" << mapping
            << ",\"timeoutMs\":" << timeoutMs << "}\n";
    return message.str();
}

std::string InputBridge::formatStartCaptureMessage(
    const std::string& api,
    const std::string& uuid,
    const std::string& displayName,
    int mapping,
    const std::string& captureMode) {
    std::ostringstream message;
    message << "{\"op\":\"start_capture\",\"api\":\"" << escapeJson(api)
            << "\",\"uuid\":\"" << escapeJson(uuid)
            << "\",\"displayName\":\"" << escapeJson(displayName)
            << "\",\"mapping\":" << mapping
            << ",\"captureMode\":\"" << escapeJson(captureMode) << "\"}\n";
    return message.str();
}

std::string InputBridge::formatPollCaptureMessage(int captureId) {
    std::ostringstream message;
    message << "{\"op\":\"poll_capture\",\"captureId\":" << captureId << "}\n";
    return message.str();
}

std::string InputBridge::formatCancelCaptureMessage(int captureId) {
    std::ostringstream message;
    message << "{\"op\":\"cancel_capture\",\"captureId\":" << captureId << "}\n";
    return message.str();
}

std::string InputBridge::formatReadInputMessage(
    const std::string& api,
    const std::string& uuid,
    const std::string& displayName,
    unsigned long long sourceInputId) {
    std::ostringstream message;
    message << "{\"op\":\"read_input\",\"api\":\"" << escapeJson(api)
            << "\",\"uuid\":\"" << escapeJson(uuid)
            << "\",\"displayName\":\"" << escapeJson(displayName)
            << "\",\"sourceInputId\":" << sourceInputId << "}\n";
    return message.str();
}

std::string InputBridge::formatLoadProfileMessage(int playerIndex, const std::string& profileName) {
    std::ostringstream message;
    message << "{\"op\":\"load_profile\",\"playerIndex\":" << playerIndex
            << ",\"profile\":\"" << escapeJson(profileName) << "\"}\n";
    return message.str();
}

InputBridgeResult InputBridge::parseResponse(std::string_view jsonLine) {
    const std::string line(jsonLine);
    InputBridgeResult result;
    result.json = line;

    std::smatch okMatch;
    if (!std::regex_search(line, okMatch, std::regex(R"json("ok"\s*:\s*(true|false))json"))) {
        return failed("Cemu input bridge returned an invalid response.");
    }
    result.ok = okMatch[1].str() == "true";

    std::smatch errorMatch;
    if (std::regex_search(line, errorMatch, std::regex(R"json("error"\s*:\s*"((?:\\.|[^"])*)")json"))) {
        result.error = unescapeJson(errorMatch[1].str());
    }

    if (!result.ok && result.error.empty()) {
        result.error = "Cemu input bridge rejected the command.";
    }
    return result;
}

std::wstring InputBridge::pipeName(unsigned long cemuProcessId) {
    return L"\\\\.\\pipe\\SkylandersEngine.Input." + std::to_wstring(cemuProcessId);
}

InputBridgeResult InputBridge::send(unsigned long cemuProcessId, const std::string& jsonLine) const {
    if (cemuProcessId == 0) {
        return failed("Cemu is not running.");
    }

    const auto pipe = pipeName(cemuProcessId);
    WaitNamedPipeW(pipe.c_str(), kPipeTimeoutMs);
    HANDLE handle = CreateFileW(
        pipe.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (handle == INVALID_HANDLE_VALUE) {
        return failed("Controller mapping requires the bundled Cemu build.");
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(handle, &mode, nullptr, nullptr);

    DWORD written = 0;
    const BOOL ok = WriteFile(
        handle,
        jsonLine.data(),
        static_cast<DWORD>(jsonLine.size()),
        &written,
        nullptr);
    FlushFileBuffers(handle);
    if (!ok || written != jsonLine.size()) {
        CloseHandle(handle);
        return failed("Unable to send input command to Cemu.");
    }

    std::string response(65536, '\0');
    DWORD read = 0;
    const BOOL readOk = ReadFile(handle, response.data(), static_cast<DWORD>(response.size() - 1), &read, nullptr);
    CloseHandle(handle);

    if (!readOk || read == 0) {
        return failed("Cemu input bridge did not return a response.");
    }

    response.resize(read);
    return parseResponse(response);
}

} // namespace skylanders
