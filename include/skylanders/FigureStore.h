#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace skylanders {

struct FigureIds {
    std::uint16_t skyId = 0;
    std::uint16_t skyVar = 0;
};

struct FigureValidationResult {
    bool valid = false;
    bool exists = false;
    std::optional<FigureIds> ids;
    std::string reason;
};

std::optional<FigureIds> readFigureIds(const std::filesystem::path& path);
FigureValidationResult validateFigureFile(
    const std::filesystem::path& path,
    std::uint16_t expectedSkyId,
    std::uint16_t expectedSkyVar);

} // namespace skylanders
