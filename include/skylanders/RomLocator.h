#pragma once

#include "skylanders/GameCatalog.h"

#include <filesystem>
#include <optional>
#include <string>

namespace skylanders {

struct RomLocatorConfig {
    std::filesystem::path romsRoot;
    std::filesystem::path persistedPathsFile;
};

struct RomValidationResult {
    bool valid = false;
    std::optional<ResolvedGameImage> image;
    std::string reason;
};

class RomLocator {
public:
    explicit RomLocator(RomLocatorConfig config);

    std::optional<ResolvedGameImage> resolve(GameId game);
    std::optional<ResolvedGameImage> promptAndPersist(GameId game, void* ownerWindow);
    RomValidationResult validateForGame(GameId game, const std::filesystem::path& path, GameImageSource source) const;

private:
    std::optional<ResolvedGameImage> resolveFromRomsFolder(GameId game) const;
    std::optional<ResolvedGameImage> resolveFromPersistedPath(GameId game);
    void persist(GameId game, const std::filesystem::path& path) const;
    void clearPersisted(GameId game) const;

    RomLocatorConfig config_;
};

} // namespace skylanders
