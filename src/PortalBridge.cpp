#include "skylanders/PortalBridge.h"

#include <cstdint>
#include <regex>
#include <sstream>
#include <stdexcept>
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

PortalCommandResult failed(std::string error) {
    PortalCommandResult result;
    result.error = std::move(error);
    return result;
}

} // namespace

PortalCommandResult PortalBridge::place(unsigned long cemuProcessId, int slot, const std::filesystem::path& figurePath) const {
    return send(cemuProcessId, formatPlaceMessage(slot, figurePath));
}

PortalCommandResult PortalBridge::placeOrCreate(
    unsigned long cemuProcessId,
    int slot,
    const std::filesystem::path& figurePath,
    std::uint16_t skyId,
    std::uint16_t skyVar) const {
    return send(cemuProcessId, formatPlaceOrCreateMessage(slot, figurePath, skyId, skyVar));
}

PortalCommandResult PortalBridge::remove(unsigned long cemuProcessId, int slot) const {
    return send(cemuProcessId, formatRemoveMessage(slot));
}

bool PortalBridge::ping(unsigned long cemuProcessId) const {
    return send(cemuProcessId, formatPingMessage()).ok;
}

std::string PortalBridge::formatPlaceMessage(int slot, const std::filesystem::path& figurePath) {
    std::ostringstream message;
    message << "{\"op\":\"place\",\"slot\":" << slot << ",\"path\":\""
            << escapeJson(figurePath.string()) << "\"}\n";
    return message.str();
}

std::string PortalBridge::formatPlaceOrCreateMessage(
    int slot,
    const std::filesystem::path& figurePath,
    std::uint16_t skyId,
    std::uint16_t skyVar) {
    std::ostringstream message;
    message << "{\"op\":\"place_or_create\",\"slot\":" << slot << ",\"path\":\""
            << escapeJson(figurePath.string()) << "\",\"skyId\":" << skyId
            << ",\"skyVar\":" << skyVar << "}\n";
    return message.str();
}

std::string PortalBridge::formatRemoveMessage(int slot) {
    std::ostringstream message;
    message << "{\"op\":\"remove\",\"slot\":" << slot << "}\n";
    return message.str();
}

std::string PortalBridge::formatPingMessage() {
    return "{\"op\":\"ping\"}\n";
}

PortalCommandResult PortalBridge::parseResponse(std::string_view jsonLine) {
    const std::string line(jsonLine);
    PortalCommandResult result;

    std::smatch okMatch;
    if (!std::regex_search(line, okMatch, std::regex(R"json("ok"\s*:\s*(true|false))json"))) {
        return failed("Cemu portal bridge returned an invalid response.");
    }
    result.ok = okMatch[1].str() == "true";

    std::smatch createdMatch;
    if (std::regex_search(line, createdMatch, std::regex(R"json("created"\s*:\s*(true|false))json"))) {
        result.created = createdMatch[1].str() == "true";
    }

    std::smatch portalSlotMatch;
    if (std::regex_search(line, portalSlotMatch, std::regex(R"json("portalSlot"\s*:\s*(\d+))json"))) {
        result.portalSlot = std::stoi(portalSlotMatch[1].str());
    }

    std::smatch errorMatch;
    if (std::regex_search(line, errorMatch, std::regex(R"json("error"\s*:\s*"((?:\\.|[^"])*)")json"))) {
        result.error = unescapeJson(errorMatch[1].str());
    }

    if (!result.ok && result.error.empty()) {
        result.error = "Cemu portal bridge rejected the command.";
    }
    return result;
}

std::wstring PortalBridge::pipeName(unsigned long cemuProcessId) {
    return L"\\\\.\\pipe\\SkylandersEngine.Portal." + std::to_wstring(cemuProcessId);
}

PortalCommandResult PortalBridge::send(unsigned long cemuProcessId, const std::string& jsonLine) const {
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
        return failed("Portal bridge unavailable for this Cemu build.");
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
        return failed("Unable to send portal command to Cemu.");
    }

    std::string response(4096, '\0');
    DWORD read = 0;
    const BOOL readOk = ReadFile(handle, response.data(), static_cast<DWORD>(response.size() - 1), &read, nullptr);
    CloseHandle(handle);

    if (!readOk || read == 0) {
        return failed("Cemu portal bridge did not return a response.");
    }

    response.resize(read);
    return parseResponse(response);
}

} // namespace skylanders
