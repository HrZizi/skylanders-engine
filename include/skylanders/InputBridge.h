#pragma once

#include <string>

namespace skylanders {

struct InputBridgeResult {
    bool ok = false;
    std::string json;
    std::string error;
};

class InputBridge {
public:
    bool ping(unsigned long cemuProcessId) const;
    InputBridgeResult listDevices(unsigned long cemuProcessId) const;
    InputBridgeResult captureNext(
        unsigned long cemuProcessId,
        const std::string& api,
        const std::string& uuid,
        const std::string& displayName,
        int mapping,
        int timeoutMs) const;
    InputBridgeResult startCapture(
        unsigned long cemuProcessId,
        const std::string& api,
        const std::string& uuid,
        const std::string& displayName,
        int mapping,
        const std::string& captureMode) const;
    InputBridgeResult pollCapture(unsigned long cemuProcessId, int captureId) const;
    InputBridgeResult cancelCapture(unsigned long cemuProcessId, int captureId) const;
    InputBridgeResult readInput(
        unsigned long cemuProcessId,
        const std::string& api,
        const std::string& uuid,
        const std::string& displayName,
        unsigned long long sourceInputId) const;
    InputBridgeResult loadProfile(unsigned long cemuProcessId, int playerIndex, const std::string& profileName) const;

    static std::string formatPingMessage();
    static std::string formatListDevicesMessage();
    static std::string formatCaptureNextMessage(
        const std::string& api,
        const std::string& uuid,
        const std::string& displayName,
        int mapping,
        int timeoutMs);
    static std::string formatStartCaptureMessage(
        const std::string& api,
        const std::string& uuid,
        const std::string& displayName,
        int mapping,
        const std::string& captureMode);
    static std::string formatPollCaptureMessage(int captureId);
    static std::string formatCancelCaptureMessage(int captureId);
    static std::string formatReadInputMessage(
        const std::string& api,
        const std::string& uuid,
        const std::string& displayName,
        unsigned long long sourceInputId);
    static std::string formatLoadProfileMessage(int playerIndex, const std::string& profileName);
    static InputBridgeResult parseResponse(std::string_view jsonLine);
    static std::wstring pipeName(unsigned long cemuProcessId);

private:
    InputBridgeResult send(unsigned long cemuProcessId, const std::string& jsonLine) const;
};

} // namespace skylanders
