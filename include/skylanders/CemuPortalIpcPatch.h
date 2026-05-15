#pragma once

// This header is intended to be copied into the Cemu-side patch/plugin project,
// not linked into SkylandersEngine.exe. It avoids cross-process code injection:
// Cemu owns its process, listens on a named pipe, then calls its own
// nsyshid::g_skyportal API from inside the emulator.

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <string>
#include <thread>

namespace skylanders_cemu_patch {

struct PortalCommand {
    enum class Op {
        Place,
        PlaceOrCreate,
        Remove,
    };

    Op op = Op::Place;
    int slot = 0;
    std::filesystem::path path;
    std::uint16_t skyId = 0;
    std::uint16_t skyVar = 0;
};

class PortalIpcServer {
public:
    using CommandHandler = void (*)(const PortalCommand&);

    explicit PortalIpcServer(CommandHandler handler);
    ~PortalIpcServer();

    void start();
    void stop();

    static std::wstring pipeNameForCurrentProcess();

private:
    void run();
    static bool parseLine(const std::string& line, PortalCommand& out);

    CommandHandler handler_ = nullptr;
    std::atomic_bool running_{false};
    std::thread thread_;
};

// Cemu-side handler sketch:
//
// void HandlePortalCommand(const PortalCommand& command) {
//   if (command.op == PortalCommand::Op::Remove) {
//     nsyshid::g_skyportal.RemoveSkylander(static_cast<uint8>(command.slot));
//     return;
//   }
//
//   if (command.op == PortalCommand::Op::PlaceOrCreate && !fs::exists(command.path)) {
//     nsyshid::g_skyportal.CreateSkylander(command.path, command.skyId, command.skyVar);
//   }
//
//   auto file = FileStream::openFile2(command.path);
//   if (!file) return;
//
//   std::vector<uint8> data;
//   file->extract(data);
//   if (data.empty()) return;
//
//   nsyshid::g_skyportal.LoadSkylander(data.data(), std::move(file));
// }
//
// static PortalIpcServer g_portalIpc(&HandlePortalCommand);
// Call g_portalIpc.start() after nsyshid is initialized and g_portalIpc.stop()
// during Cemu shutdown.

} // namespace skylanders_cemu_patch
