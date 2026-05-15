#include "skylanders/GameCatalog.h"

#include <algorithm>
#include <cwctype>

namespace skylanders {
namespace {

const std::vector<GameCatalogEntry> kCatalog = {
    {
        GameId::SpyrosAdventure,
        L"Skylanders: Spyro's Adventure",
        L"SpyrosAdventure.exe",
        {L"SpyrosAdventure.exe", L"SpyroAdventure.exe", L"SkylandersSpyrosAdventure.exe"},
        {{0x0005000010142D00ull, L"JPN"}},
        {},
        {"skylanders", "spyro", "spyros", "adventure"},
        {L".wud", L".wux", L".wua", L".iso", L".rpx", L".tmd"},
    },
    {
        GameId::Giants,
        L"Skylanders: Giants",
        L"Giants.exe",
        {L"Giants.exe", L"SkylandersGiants.exe"},
        {{0x000500001010D700ull, L"USA"}, {0x0005000010116000ull, L"EUR"}},
        {},
        {"skylanders", "giants"},
        {L".wud", L".wux", L".wua", L".iso", L".rpx", L".tmd"},
    },
    {
        GameId::SwapForce,
        L"Skylanders: Swap Force",
        L"SwapForce.exe",
        {L"SwapForce.exe", L"SkylandersSwapForce.exe", L"Swap Force.exe"},
        {{0x0005000010139200ull, L"USA"}, {0x0005000010140400ull, L"EUR"}},
        {},
        {"skylanders", "swap", "force", "swapforce"},
        {L".wud", L".wux", L".wua", L".iso", L".rpx", L".tmd"},
    },
    {
        GameId::TrapTeam,
        L"Skylanders: Trap Team",
        L"TrapTeam.exe",
        {L"TrapTeam.exe", L"SkylandersTrapTeam.exe", L"Trap Team.exe"},
        {{0x000500001017C600ull, L"USA"}, {0x0005000010181F00ull, L"EUR"}},
        {},
        {"skylanders", "trap", "team", "trapteam", "traptanium"},
        {L".wud", L".wux", L".wua", L".iso", L".rpx", L".tmd"},
    },
    {
        GameId::SuperChargers,
        L"Skylanders: SuperChargers",
        L"SuperChargers.exe",
        {L"SuperChargers.exe", L"SkylandersSuperChargers.exe", L"Super Chargers.exe"},
        {{0x00050000101B8500ull, L"USA"}, {0x00050000101BFC00ull, L"EUR"}},
        {},
        {"skylanders", "superchargers", "super", "chargers"},
        {L".wud", L".wux", L".wua", L".iso", L".rpx", L".tmd"},
    },
    {
        GameId::Imaginators,
        L"Skylanders: Imaginators",
        L"Imaginators.exe",
        {L"Imaginators.exe", L"SkylandersImaginators.exe"},
        {{0x00050000101F4D00ull, L"USA"}, {0x00050000101FB100ull, L"EUR"}},
        {},
        {"skylanders", "imaginators"},
        {L".wud", L".wux", L".wua", L".iso", L".rpx", L".tmd"},
    },
};

} // namespace

std::wstring toStubStem(std::filesystem::path path) {
    auto stem = path.stem().wstring();
    std::transform(stem.begin(), stem.end(), stem.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    stem.erase(std::remove(stem.begin(), stem.end(), L' '), stem.end());
    stem.erase(std::remove(stem.begin(), stem.end(), L'_'), stem.end());
    stem.erase(std::remove(stem.begin(), stem.end(), L'-'), stem.end());
    stem.erase(std::remove(stem.begin(), stem.end(), L'\''), stem.end());
    return stem;
}

std::wstring gameIdToString(GameId game) {
    switch (game) {
    case GameId::SpyrosAdventure: return L"SpyrosAdventure";
    case GameId::Giants: return L"Giants";
    case GameId::SwapForce: return L"SwapForce";
    case GameId::TrapTeam: return L"TrapTeam";
    case GameId::SuperChargers: return L"SuperChargers";
    case GameId::Imaginators: return L"Imaginators";
    case GameId::Unknown: return L"Unknown";
    }
    return L"Unknown";
}

std::string gameIdToConfigKey(GameId game) {
    switch (game) {
    case GameId::SpyrosAdventure: return "SpyrosAdventure";
    case GameId::Giants: return "Giants";
    case GameId::SwapForce: return "SwapForce";
    case GameId::TrapTeam: return "TrapTeam";
    case GameId::SuperChargers: return "SuperChargers";
    case GameId::Imaginators: return "Imaginators";
    case GameId::Unknown: return "Unknown";
    }
    return "Unknown";
}

const std::vector<GameCatalogEntry>& gameCatalog() {
    return kCatalog;
}

const GameCatalogEntry* findGameCatalogEntry(GameId game) {
    const auto& catalog = gameCatalog();
    const auto it = std::find_if(catalog.begin(), catalog.end(), [game](const auto& entry) {
        return entry.game == game;
    });
    return it == catalog.end() ? nullptr : &*it;
}

const GameCatalogEntry* findGameCatalogEntryByStub(const std::filesystem::path& stubPath) {
    const auto stem = toStubStem(stubPath);
    for (const auto& entry : gameCatalog()) {
        for (const auto& stub : entry.stubNames) {
            if (toStubStem(stub) == stem) {
                return &entry;
            }
        }
    }
    return nullptr;
}

std::optional<GameTitleId> findBaseTitleId(GameId game, std::uint64_t titleId) {
    const auto* entry = findGameCatalogEntry(game);
    if (!entry) {
        return std::nullopt;
    }

    const auto it = std::find_if(entry->acceptedBaseTitleIds.begin(), entry->acceptedBaseTitleIds.end(), [titleId](const auto& candidate) {
        return candidate.value == titleId;
    });
    if (it == entry->acceptedBaseTitleIds.end()) {
        return std::nullopt;
    }
    return *it;
}

} // namespace skylanders
