#include "skylanders/EmulatorCore.h"
#include "skylanders/FigureStore.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

#if SKYLANDERS_HAS_CEMU
#include "Cafe/CafeSystem.h"
#include "Cafe/OS/libs/nsyshid/Skylander.h"
#include "Cafe/TitleList/TitleInfo.h"
#include "Cafe/TitleList/TitleList.h"
#include "Common/FileStream.h"

#ifdef ENABLE_VULKAN
#include "Cafe/HW/Latte/Renderer/Vulkan/VulkanAPI.h"
#endif
#endif

namespace skylanders {
namespace {

constexpr std::size_t kSkyFigureSize = 0x400;

std::vector<std::uint8_t> readFigureBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open Skylander figure file.");
    }

    std::vector<std::uint8_t> bytes(kSkyFigureSize);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (file.gcount() != static_cast<std::streamsize>(bytes.size())) {
        throw std::runtime_error("Skylander figure file is smaller than 0x400 bytes.");
    }

    return bytes;
}

std::uint16_t readLe16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
}

} // namespace

EmulatorCore::EmulatorCore(void* renderWindowHandle)
    : renderWindowHandle_(renderWindowHandle) {
}

EmulatorCore::~EmulatorCore() {
    stop();
}

bool EmulatorCore::available() const noexcept {
#if SKYLANDERS_HAS_CEMU
    return true;
#else
    return false;
#endif
}

bool EmulatorCore::start(const GameLaunchSpec& spec) {
    std::scoped_lock lock(mutex_);
    if (running_) {
        return true;
    }

    if (!initializeCemu(spec)) {
        return false;
    }

    if (!bootTitle(spec)) {
        return false;
    }

    activeSpec_ = spec;
    running_ = true;
    return true;
}

void EmulatorCore::stop() {
    std::scoped_lock lock(mutex_);
    if (!running_) {
        return;
    }

#if SKYLANDERS_HAS_CEMU
    CafeSystem::ShutdownTitle();
#endif

    for (auto& figure : placedFigures_) {
        figure.reset();
    }
    running_ = false;
    activeSpec_.reset();
}

bool EmulatorCore::placeFigure(int uiSlot, const std::filesystem::path& filePath) {
    std::scoped_lock lock(mutex_);
    if (!running_ || uiSlot < 0 || uiSlot >= static_cast<int>(placedFigures_.size())) {
        return false;
    }

    try {
        return placeFigureInternal(uiSlot, filePath);
    } catch (const std::exception&) {
        return false;
    }
}

bool EmulatorCore::placeOrCreateFigure(
    int uiSlot,
    const std::filesystem::path& filePath,
    std::uint16_t skyId,
    std::uint16_t skyVar) {
    std::scoped_lock lock(mutex_);
    if (!running_ || uiSlot < 0 || uiSlot >= static_cast<int>(placedFigures_.size())) {
        return false;
    }

    try {
        std::error_code ec;
        if (!std::filesystem::exists(filePath, ec)) {
#if SKYLANDERS_HAS_CEMU
            std::filesystem::create_directories(filePath.parent_path());
            if (!nsyshid::g_skyportal.CreateSkylander(filePath, skyId, skyVar)) {
                return false;
            }
#else
            return false;
#endif
        }

        const auto validation = validateFigureFile(filePath, skyId, skyVar);
        if (!validation.valid) {
            return false;
        }

        return placeFigureInternal(uiSlot, filePath);
    } catch (const std::exception&) {
        return false;
    }
}

bool EmulatorCore::removeFigure(int uiSlot) {
    std::scoped_lock lock(mutex_);
    if (uiSlot < 0 || uiSlot >= static_cast<int>(placedFigures_.size())) {
        return false;
    }

    return removeFigureInternal(uiSlot);
}

bool EmulatorCore::isRunning() const noexcept {
    std::scoped_lock lock(mutex_);
    return running_;
}

bool EmulatorCore::initializeCemu(const GameLaunchSpec&) {
    if (initialized_) {
        return true;
    }

#if SKYLANDERS_HAS_CEMU
#ifdef ENABLE_VULKAN
    if (!InitializeGlobalVulkan()) {
        return false;
    }
#endif

    // Upstream Cemu keeps the full common bootstrap in src/main.cpp as
    // CemuCommonInit(). The next upstream patch should move that function into
    // a linkable CemuCoreBootstrap target; until then, this class owns the call
    // site and render-host boundary.
    CafeSystem::Initialize();
    initialized_ = true;
    return true;
#else
    return false;
#endif
}

bool EmulatorCore::bootTitle(const GameLaunchSpec& spec) {
#if SKYLANDERS_HAS_CEMU
    if (spec.gamePath.empty()) {
        return false;
    }

    TitleInfo launchTitle{ spec.gamePath };
    if (launchTitle.IsValid()) {
        CafeTitleList::AddTitleFromPath(spec.gamePath);

        TitleId baseTitleId{};
        if (!CafeTitleList::FindBaseTitleId(launchTitle.GetAppTitleId(), baseTitleId)) {
            return false;
        }

        if (CafeSystem::PrepareForegroundTitle(baseTitleId) != CafeSystem::PREPARE_STATUS_CODE::SUCCESS) {
            return false;
        }
    } else {
        const CafeTitleFileType fileType = DetermineCafeSystemFileType(spec.gamePath);
        if (fileType != CafeTitleFileType::RPX && fileType != CafeTitleFileType::ELF) {
            return false;
        }

        if (CafeSystem::PrepareForegroundTitleFromStandaloneRPX(spec.gamePath) != CafeSystem::PREPARE_STATUS_CODE::SUCCESS) {
            return false;
        }
    }

    CafeSystem::LaunchForegroundTitle();
    return true;
#else
    (void)spec;
    return false;
#endif
}

bool EmulatorCore::placeFigureInternal(int uiSlot, const std::filesystem::path& filePath) {
    const auto bytes = readFigureBytes(filePath);
    const auto figureId = readLe16(bytes, 0x10);
    const auto variantId = readLe16(bytes, 0x1C);

#if SKYLANDERS_HAS_CEMU
    removeFigureInternal(uiSlot);

    std::unique_ptr<FileStream> skyFile(FileStream::openFile2(filePath, true));
    if (!skyFile) {
        return false;
    }

    std::array<uint8, nsyshid::SKY_FIGURE_SIZE> cemuBytes{};
    std::copy(bytes.begin(), bytes.end(), cemuBytes.begin());

    const uint8 portalSlot = nsyshid::g_skyportal.LoadSkylander(cemuBytes.data(), std::move(skyFile));
    if (portalSlot == 0xFF) {
        return false;
    }
    placedFigures_[uiSlot] = PlacedFigure{
        uiSlot,
        static_cast<int>(portalSlot),
        filePath,
        figureId,
        variantId,
    };
    return true;
#else
    (void)uiSlot;
    (void)figureId;
    (void)variantId;
    return false;
#endif
}

bool EmulatorCore::removeFigureInternal(int uiSlot) {
    auto& figure = placedFigures_[uiSlot];
    if (!figure) {
        return true;
    }

#if SKYLANDERS_HAS_CEMU
    const bool removed = nsyshid::g_skyportal.RemoveSkylander(static_cast<uint8>(figure->portalSlot));
    if (!removed) {
        return false;
    }
#endif

    figure.reset();
    return true;
}

} // namespace skylanders
