// Reference implementation for the Cemu-side patch described by
// include/skylanders/CemuPortalIpcPatch.h. This file is intentionally kept in
// docs/ because it depends on Cemu internals and should be copied into a Cemu
// fork or plugin project, not compiled into SkylandersEngine.exe.

#include "CemuPortalIpcPatch.h"

#include <cstdint>
#include <regex>
#include <sstream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace skylanders_cemu_patch {
namespace {

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

} // namespace

PortalIpcServer::PortalIpcServer(CommandHandler handler)
    : handler_(handler) {}

PortalIpcServer::~PortalIpcServer() {
    stop();
}

void PortalIpcServer::start() {
    if (running_.exchange(true)) {
        return;
    }
    thread_ = std::thread(&PortalIpcServer::run, this);
}

void PortalIpcServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    const auto pipeName = pipeNameForCurrentProcess();
    HANDLE wake = CreateFileW(pipeName.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (wake != INVALID_HANDLE_VALUE) {
        CloseHandle(wake);
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

std::wstring PortalIpcServer::pipeNameForCurrentProcess() {
    return L"\\\\.\\pipe\\SkylandersEngine.Portal." + std::to_wstring(GetCurrentProcessId());
}

void PortalIpcServer::run() {
    const auto pipeName = pipeNameForCurrentProcess();
    while (running_) {
        HANDLE pipe = CreateNamedPipeW(
            pipeName.c_str(),
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,
            4096,
            4096,
            0,
            nullptr);

        if (pipe == INVALID_HANDLE_VALUE) {
            return;
        }

        const BOOL connected = ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED;
        if (connected) {
            std::string line(4096, '\0');
            DWORD read = 0;
            if (ReadFile(pipe, line.data(), static_cast<DWORD>(line.size() - 1), &read, nullptr) && read > 0) {
                line.resize(read);
                PortalCommand command;
                if (parseLine(line, command) && handler_) {
                    handler_(command);
                }
            }
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}

bool PortalIpcServer::parseLine(const std::string& line, PortalCommand& out) {
    std::smatch opMatch;
    if (!std::regex_search(line, opMatch, std::regex(R"json("op"\s*:\s*"(place|place_or_create|remove)")json"))) {
        return false;
    }

    const auto op = opMatch[1].str();
    out.op = op == "remove"
        ? PortalCommand::Op::Remove
        : (op == "place_or_create" ? PortalCommand::Op::PlaceOrCreate : PortalCommand::Op::Place);

    std::smatch slotMatch;
    if (std::regex_search(line, slotMatch, std::regex(R"json("slot"\s*:\s*(\d+))json"))) {
        out.slot = std::stoi(slotMatch[1].str());
    }

    std::smatch pathMatch;
    if (std::regex_search(line, pathMatch, std::regex(R"json("path"\s*:\s*"((?:\\.|[^"])*)")json"))) {
        out.path = unescapeJson(pathMatch[1].str());
    }

    std::smatch skyIdMatch;
    if (std::regex_search(line, skyIdMatch, std::regex(R"json("skyId"\s*:\s*(\d+))json"))) {
        out.skyId = static_cast<std::uint16_t>(std::stoi(skyIdMatch[1].str()));
    }

    std::smatch skyVarMatch;
    if (std::regex_search(line, skyVarMatch, std::regex(R"json("skyVar"\s*:\s*(\d+))json"))) {
        out.skyVar = static_cast<std::uint16_t>(std::stoi(skyVarMatch[1].str()));
    }

    return out.op == PortalCommand::Op::Remove || !out.path.empty();
}

} // namespace skylanders_cemu_patch
