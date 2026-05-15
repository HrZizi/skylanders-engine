#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace skylanders {

enum class GameId {
    Unknown,
    SpyrosAdventure,
    Giants,
    SwapForce,
    TrapTeam,
    SuperChargers,
    Imaginators,
};

enum class GameImageSource {
    BundledRomsFolder,
    PersistedExternalPath,
};

struct GameTitleId {
    std::uint64_t value = 0;
    std::wstring region;
};

struct GameCatalogEntry {
    GameId game = GameId::Unknown;
    std::wstring displayName;
    std::wstring defaultStubExe;
    std::vector<std::wstring> stubNames;
    std::vector<GameTitleId> acceptedBaseTitleIds;
    std::vector<GameTitleId> acceptedUpdateTitleIds;
    std::vector<std::string> searchTerms;
    std::vector<std::wstring> acceptedExtensions;
};

struct ResolvedGameImage {
    std::filesystem::path path;
    std::uint64_t titleId = 0;
    std::wstring region;
    GameImageSource source = GameImageSource::BundledRomsFolder;
};

std::wstring toStubStem(std::filesystem::path path);
std::wstring gameIdToString(GameId game);
std::string gameIdToConfigKey(GameId game);

const std::vector<GameCatalogEntry>& gameCatalog();
const GameCatalogEntry* findGameCatalogEntry(GameId game);
const GameCatalogEntry* findGameCatalogEntryByStub(const std::filesystem::path& stubPath);
std::optional<GameTitleId> findBaseTitleId(GameId game, std::uint64_t titleId);

} // namespace skylanders
