#pragma once

#include "skylanders/GameCatalog.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace skylanders {

enum class ToyKind {
    Skylander,
    Item,
    Trap,
    Vehicle,
    Trophy,
};

struct CharacterCatalogEntry {
    std::string id;
    std::string displayName;
    std::string cemuName;
    ToyKind toyKind = ToyKind::Skylander;
    std::string element;
    std::string role;
    std::string baseName;
    std::string variantName;
    bool isVariant = false;
    std::string assetKey;
    GameId releaseGame = GameId::Unknown;
    std::vector<GameId> games;
    std::uint16_t skyId = 0;
    std::uint16_t skyVar = 0;
    std::string sourceUrl;
    std::string sclSearchName;
    std::string wikiIconTitle;
    std::string swapHalf;
    std::string swapGroup;
    std::filesystem::path renderPath;
    std::filesystem::path iconPath;
    std::filesystem::path figurePath;
};

const std::vector<CharacterCatalogEntry>& characterCatalog();
const CharacterCatalogEntry* findCharacter(std::string_view id);
std::string toyKindToString(ToyKind toyKind);
std::string characterCatalogJson();
bool isExcludedVariantCategory(std::string_view category);

} // namespace skylanders
