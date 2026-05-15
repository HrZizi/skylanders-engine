#include "skylanders/CharacterCatalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>

namespace skylanders {
namespace {

using G = GameId;
using T = ToyKind;

std::string jsonEscape(std::string value) {
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

std::string gameKey(G game) {
    return gameIdToConfigKey(game);
}

std::vector<G> compatibleFrom(G releaseGame) {
    constexpr std::array<G, 6> order = {
        G::SpyrosAdventure,
        G::Giants,
        G::SwapForce,
        G::TrapTeam,
        G::SuperChargers,
        G::Imaginators,
    };

    auto it = std::find(order.begin(), order.end(), releaseGame);
    if (it == order.end()) {
        return {};
    }
    return std::vector<G>(it, order.end());
}

CharacterCatalogEntry character(
    std::string id,
    std::string displayName,
    std::string cemuName,
    T toyKind,
    std::string element,
    std::string role,
    std::string baseName,
    std::string variantName,
    bool isVariant,
    std::string assetKey,
    G releaseGame,
    std::uint16_t skyId,
    std::uint16_t skyVar,
    std::string sourcePage,
    std::string sclSearchName,
    std::string wikiIconTitle,
    std::string swapHalf = {},
    std::string swapGroup = {}) {
    const auto game = gameKey(releaseGame);
    const auto key = assetKey.empty() ? id : assetKey;
    return {
        std::move(id),
        std::move(displayName),
        std::move(cemuName),
        toyKind,
        std::move(element),
        std::move(role),
        std::move(baseName),
        std::move(variantName),
        isVariant,
        key,
        releaseGame,
        compatibleFrom(releaseGame),
        skyId,
        skyVar,
        std::move(sourcePage),
        std::move(sclSearchName),
        std::move(wikiIconTitle),
        std::move(swapHalf),
        std::move(swapGroup),
        std::filesystem::path("characters") / "renders" / game / (key + ".png"),
        std::filesystem::path("characters") / "icons" / game / (key + ".png"),
        std::filesystem::path(game) / (key + ".sky"),
    };
}

const std::vector<CharacterCatalogEntry> kCharacters = {
#include "GeneratedCharacterCatalog.inc"
};

} // namespace

const std::vector<CharacterCatalogEntry>& characterCatalog() {
    return kCharacters;
}

const CharacterCatalogEntry* findCharacter(std::string_view id) {
    const auto it = std::find_if(kCharacters.begin(), kCharacters.end(), [id](const auto& entry) {
        return entry.id == id;
    });
    return it == kCharacters.end() ? nullptr : &*it;
}

std::string toyKindToString(ToyKind toyKind) {
    switch (toyKind) {
    case ToyKind::Skylander: return "Skylander";
    case ToyKind::Item: return "Item";
    case ToyKind::Trap: return "Trap";
    case ToyKind::Vehicle: return "Vehicle";
    case ToyKind::Trophy: return "Trophy";
    }
    return "Skylander";
}

std::string characterCatalogJson() {
    std::ostringstream json;
    json << "[";
    bool first = true;
    for (const auto& entry : kCharacters) {
        if (!first) {
            json << ",";
        }
        first = false;
        json << "{";
        json << "\"id\":\"" << jsonEscape(entry.id) << "\",";
        json << "\"displayName\":\"" << jsonEscape(entry.displayName) << "\",";
        json << "\"cemuName\":\"" << jsonEscape(entry.cemuName) << "\",";
        json << "\"toyKind\":\"" << jsonEscape(toyKindToString(entry.toyKind)) << "\",";
        json << "\"element\":\"" << jsonEscape(entry.element) << "\",";
        json << "\"role\":\"" << jsonEscape(entry.role) << "\",";
        json << "\"baseName\":\"" << jsonEscape(entry.baseName) << "\",";
        json << "\"variantName\":\"" << jsonEscape(entry.variantName) << "\",";
        json << "\"isVariant\":" << (entry.isVariant ? "true" : "false") << ",";
        json << "\"assetKey\":\"" << jsonEscape(entry.assetKey) << "\",";
        json << "\"releaseGame\":\"" << jsonEscape(gameKey(entry.releaseGame)) << "\",";
        json << "\"games\":[";
        for (std::size_t i = 0; i < entry.games.size(); ++i) {
            if (i != 0) {
                json << ",";
            }
            json << "\"" << jsonEscape(gameKey(entry.games[i])) << "\"";
        }
        json << "],";
        json << "\"skyId\":" << entry.skyId << ",";
        json << "\"skyVar\":" << entry.skyVar << ",";
        json << "\"sourceUrl\":\"" << jsonEscape(entry.sourceUrl) << "\",";
        json << "\"sclSearchName\":\"" << jsonEscape(entry.sclSearchName) << "\",";
        json << "\"wikiIconTitle\":\"" << jsonEscape(entry.wikiIconTitle) << "\",";
        if (!entry.swapHalf.empty()) {
            json << "\"swapHalf\":\"" << jsonEscape(entry.swapHalf) << "\",";
        }
        if (!entry.swapGroup.empty()) {
            json << "\"swapGroup\":\"" << jsonEscape(entry.swapGroup) << "\",";
        }
        json << "\"renderPath\":\"" << jsonEscape(entry.renderPath.generic_string()) << "\",";
        json << "\"iconPath\":\"" << jsonEscape(entry.iconPath.generic_string()) << "\",";
        json << "\"imagePath\":\"" << jsonEscape(entry.renderPath.generic_string()) << "\",";
        json << "\"figurePath\":\"" << jsonEscape(entry.figurePath.generic_string()) << "\"";
        json << "}";
    }
    json << "]";
    return json.str();
}

bool isExcludedVariantCategory(std::string_view category) {
    std::string normalized(category);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return normalized.find("in-game variant") != std::string::npos ||
        normalized.find("in game variant") != std::string::npos ||
        normalized.find("chase variant") != std::string::npos;
}

} // namespace skylanders
