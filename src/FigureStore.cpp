#include "skylanders/FigureStore.h"

#include <array>
#include <fstream>
#include <sstream>

namespace skylanders {
namespace {

constexpr std::size_t kSkyFigureSize = 0x400;

std::uint16_t readLe16(const std::array<unsigned char, kSkyFigureSize>& bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
}

std::string idMessage(const FigureIds& ids) {
    std::ostringstream out;
    out << "found skyId " << ids.skyId << ", variant " << ids.skyVar;
    return out.str();
}

} // namespace

std::optional<FigureIds> readFigureIds(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    std::array<unsigned char, kSkyFigureSize> bytes{};
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (file.gcount() != static_cast<std::streamsize>(bytes.size())) {
        return std::nullopt;
    }

    return FigureIds{
        readLe16(bytes, 0x10),
        readLe16(bytes, 0x1C),
    };
}

FigureValidationResult validateFigureFile(
    const std::filesystem::path& path,
    std::uint16_t expectedSkyId,
    std::uint16_t expectedSkyVar) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return {false, false, std::nullopt, "Figure file does not exist."};
    }

    const auto ids = readFigureIds(path);
    if (!ids) {
        return {false, true, std::nullopt, "Figure file is not a readable 0x400-byte Skylander dump."};
    }

    if (ids->skyId != expectedSkyId || ids->skyVar != expectedSkyVar) {
        return {false, true, ids, "Figure file belongs to a different character: " + idMessage(*ids) + "."};
    }

    return {true, true, ids, {}};
}

} // namespace skylanders
