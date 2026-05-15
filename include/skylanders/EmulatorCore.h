#pragma once

#include "skylanders/ProcessManager.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>

namespace skylanders {

struct PlacedFigure {
    int uiSlot = -1;
    int portalSlot = -1;
    std::filesystem::path path;
    std::uint16_t figureId = 0;
    std::uint16_t variantId = 0;
};

class EmulatorCore {
public:
    explicit EmulatorCore(void* renderWindowHandle = nullptr);
    ~EmulatorCore();

    EmulatorCore(const EmulatorCore&) = delete;
    EmulatorCore& operator=(const EmulatorCore&) = delete;

    bool available() const noexcept;
    bool start(const GameLaunchSpec& spec);
    void stop();

    bool placeFigure(int uiSlot, const std::filesystem::path& filePath);
    bool placeOrCreateFigure(
        int uiSlot,
        const std::filesystem::path& filePath,
        std::uint16_t skyId,
        std::uint16_t skyVar);
    bool removeFigure(int uiSlot);
    bool isRunning() const noexcept;

private:
    bool initializeCemu(const GameLaunchSpec& spec);
    bool bootTitle(const GameLaunchSpec& spec);
    bool placeFigureInternal(int uiSlot, const std::filesystem::path& filePath);
    bool removeFigureInternal(int uiSlot);

    void* renderWindowHandle_ = nullptr;
    bool initialized_ = false;
    bool running_ = false;
    std::optional<GameLaunchSpec> activeSpec_;
    std::array<std::optional<PlacedFigure>, 16> placedFigures_{};
    mutable std::mutex mutex_;
};

} // namespace skylanders
