#include "skylanders/RomLocator.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cwctype>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>

#if SKYLANDERS_HAS_CEMU
#include "Cafe/TitleList/TitleInfo.h"
#endif

namespace skylanders {
namespace {

std::string narrow(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int length = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string out(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), length, nullptr, nullptr);
    return out;
}

std::wstring widen(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring out(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), length);
    return out;
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("failed to read " + path.string());
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::wstring lowerExtension(const std::filesystem::path& path) {
    auto ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return ext;
}

std::string normalizeText(std::string value) {
    std::string out;
    out.reserve(value.size());
    bool lastWasSpace = true;
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
            lastWasSpace = false;
        } else if (!lastWasSpace) {
            out.push_back(' ');
            lastWasSpace = true;
        }
    }
    if (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

bool pathLooksLikeGame(const GameCatalogEntry& entry, const std::filesystem::path& path) {
    const auto haystack = normalizeText(path.stem().string() + " " + path.parent_path().filename().string());
    int matched = 0;
    for (const auto& term : entry.searchTerms) {
        const auto normalized = normalizeText(term);
        if (!normalized.empty() && haystack.find(normalized) != std::string::npos) {
            ++matched;
        }
    }
    return matched >= 2 || (entry.game == GameId::Imaginators && matched >= 1);
}

std::optional<std::uint64_t> parseTitleIdFromAppXml(const std::filesystem::path& appXml) {
    if (!std::filesystem::exists(appXml)) {
        return std::nullopt;
    }

    const auto text = readFile(appXml);
    std::smatch match;
    const std::regex pattern(R"(<\s*title_id\s*>\s*([0-9a-fA-F]+)\s*<\s*/\s*title_id\s*>)", std::regex::icase);
    if (!std::regex_search(text, match, pattern)) {
        return std::nullopt;
    }
    return std::stoull(match[1].str(), nullptr, 16);
}

std::optional<std::filesystem::path> titleRootForPath(const std::filesystem::path& path) {
    std::error_code ec;
    if (std::filesystem::is_directory(path, ec)) {
        if (std::filesystem::exists(path / "code" / "app.xml", ec) &&
            std::filesystem::exists(path / "content", ec) &&
            std::filesystem::exists(path / "meta", ec)) {
            return path;
        }
        return std::nullopt;
    }

    if (lowerExtension(path) == L".rpx") {
        auto parent = path.parent_path();
        if (toStubStem(parent.filename()) == L"code") {
            auto root = parent.parent_path();
            if (std::filesystem::exists(root / "code" / "app.xml", ec) &&
                std::filesystem::exists(root / "content", ec) &&
                std::filesystem::exists(root / "meta", ec)) {
                return root;
            }
        }
    }

    return std::nullopt;
}

bool hasWuxMagic(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    const unsigned char expected[8] = {0x57, 0x55, 0x58, 0x30, 0x2E, 0xD0, 0x99, 0x10};
    unsigned char actual[8]{};
    in.read(reinterpret_cast<char*>(actual), sizeof(actual));
    return in.gcount() == sizeof(actual) && std::equal(std::begin(expected), std::end(expected), std::begin(actual));
}

bool hasWudMagic(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }

    unsigned char magic1[4]{};
    in.read(reinterpret_cast<char*>(magic1), sizeof(magic1));
    const unsigned char expected1[4] = {0x57, 0x55, 0x50, 0x2D};
    if (in.gcount() != sizeof(magic1) || !std::equal(std::begin(expected1), std::end(expected1), std::begin(magic1))) {
        return false;
    }

    in.seekg(0x10000, std::ios::beg);
    unsigned char magic2[4]{};
    in.read(reinterpret_cast<char*>(magic2), sizeof(magic2));
    const unsigned char expected2[4] = {0xCC, 0x54, 0x9E, 0xB9};
    return in.gcount() == sizeof(magic2) && std::equal(std::begin(expected2), std::end(expected2), std::begin(magic2));
}

bool isRecognizedContainer(const std::filesystem::path& path) {
    const auto ext = lowerExtension(path);
    if (ext == L".wux") {
        return hasWuxMagic(path);
    }
    if (ext == L".wud") {
        return hasWudMagic(path);
    }
    if (ext == L".iso" || ext == L".wua") {
        return true;
    }
    return false;
}

std::string escapeJson(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char ch : value) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

std::string unescapeJson(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    bool escaping = false;
    for (const char ch : value) {
        if (escaping) {
            switch (ch) {
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default: out.push_back(ch); break;
            }
            escaping = false;
        } else if (ch == '\\') {
            escaping = true;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::map<std::string, std::filesystem::path> loadPersistedPaths(const std::filesystem::path& path) {
    std::map<std::string, std::filesystem::path> paths;
    if (!std::filesystem::exists(path)) {
        return paths;
    }

    const auto text = readFile(path);
    const std::regex entryPattern(R"json("([^"]+)"\s*:\s*"((?:\\.|[^"])*)")json");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), entryPattern); it != std::sregex_iterator(); ++it) {
        paths[(*it)[1].str()] = widen(unescapeJson((*it)[2].str()));
    }
    return paths;
}

void savePersistedPaths(const std::filesystem::path& path, const std::map<std::string, std::filesystem::path>& paths) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("failed to write " + path.string());
    }

    out << "{\n";
    bool first = true;
    for (const auto& [game, file] : paths) {
        if (!first) {
            out << ",\n";
        }
        first = false;
        out << "  \"" << escapeJson(game) << "\": \"" << escapeJson(narrow(file.wstring())) << "\"";
    }
    out << "\n}\n";
}

std::wstring titleIdHex(std::uint64_t titleId) {
    std::wostringstream out;
    out << std::hex << std::nouppercase << std::setw(16) << std::setfill(L'0') << titleId;
    return out.str();
}

void showValidationError(GameId game, const std::filesystem::path& path, const std::string& reason) {
    const auto* entry = findGameCatalogEntry(game);
    const auto title = entry ? entry->displayName : L"Skylanders";
    std::wstring message = L"The selected file is not a valid ROM for " + title + L".\n\n";
    message += path.wstring() + L"\n\n";
    message += widen(reason);
    MessageBoxW(nullptr, message.c_str(), L"Skylanders Engine", MB_OK | MB_ICONERROR);
}

} // namespace

RomLocator::RomLocator(RomLocatorConfig config)
    : config_(std::move(config)) {}

std::optional<ResolvedGameImage> RomLocator::resolve(GameId game) {
    if (auto bundled = resolveFromRomsFolder(game)) {
        return bundled;
    }
    return resolveFromPersistedPath(game);
}

std::optional<ResolvedGameImage> RomLocator::promptAndPersist(GameId game, void* ownerWindow) {
    wchar_t buffer[MAX_PATH * 8]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = static_cast<HWND>(ownerWindow);
    dialog.lpstrFilter =
        L"Wii U games (*.wud;*.wux;*.wua;*.iso;*.rpx;title.tmd)\0*.wud;*.wux;*.wua;*.iso;*.rpx;title.tmd\0"
        L"All files (*.*)\0*.*\0";
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = static_cast<DWORD>(std::size(buffer));
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrTitle = L"Select Skylanders Wii U game file";

    if (!GetOpenFileNameW(&dialog)) {
        return std::nullopt;
    }

    const std::filesystem::path path(buffer);
    auto validation = validateForGame(game, path, GameImageSource::PersistedExternalPath);
    if (!validation.valid || !validation.image) {
        showValidationError(game, path, validation.reason);
        return std::nullopt;
    }

    persist(game, validation.image->path);
    return validation.image;
}

RomValidationResult RomLocator::validateForGame(GameId game, const std::filesystem::path& path, GameImageSource source) const {
    const auto* entry = findGameCatalogEntry(game);
    if (!entry) {
        return {false, std::nullopt, "unknown game"};
    }

    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return {false, std::nullopt, "file does not exist"};
    }

    auto makeImage = [&](const GameTitleId& title) {
        return ResolvedGameImage{
            std::filesystem::weakly_canonical(path, ec),
            title.value,
            title.region,
            source,
        };
    };

    if (auto root = titleRootForPath(path)) {
        try {
            const auto titleId = parseTitleIdFromAppXml(*root / "code" / "app.xml");
            if (!titleId) {
                return {false, std::nullopt, "code/app.xml does not contain a title_id"};
            }
            if (auto accepted = findBaseTitleId(game, *titleId)) {
                ResolvedGameImage image = makeImage(*accepted);
                image.path = std::filesystem::weakly_canonical(path, ec);
                return {true, image, {}};
            }
            return {false, std::nullopt, "detected title ID 0x" + narrow(titleIdHex(*titleId)) + " is not accepted for this game"};
        } catch (const std::exception& ex) {
            return {false, std::nullopt, ex.what()};
        }
    }

#if SKYLANDERS_HAS_CEMU
    {
        TitleInfo titleInfo(path);
        if (titleInfo.IsValid()) {
            const auto titleId = titleInfo.GetAppTitleId();
            if (auto accepted = findBaseTitleId(game, titleId)) {
                return {true, makeImage(*accepted), {}};
            }
            return {false, std::nullopt, "detected title ID 0x" + narrow(titleIdHex(titleId)) + " is not accepted for this game"};
        }
    }
#endif

    if (isRecognizedContainer(path)) {
        if (!pathLooksLikeGame(*entry, path)) {
            return {false, std::nullopt, "single-file image title ID is not readable in this build, and the file name does not match the selected game"};
        }
        if (entry->acceptedBaseTitleIds.empty()) {
            return {false, std::nullopt, "game catalog has no accepted title IDs"};
        }
        return {true, makeImage(entry->acceptedBaseTitleIds.front()), "accepted by single-file image fallback"};
    }

    return {false, std::nullopt, "unsupported game image format"};
}

std::optional<ResolvedGameImage> RomLocator::resolveFromRomsFolder(GameId game) const {
    std::error_code ec;
    if (!std::filesystem::exists(config_.romsRoot, ec)) {
        return std::nullopt;
    }

    std::vector<std::filesystem::path> candidates;
    for (std::filesystem::recursive_directory_iterator it(config_.romsRoot, ec), end; !ec && it != end; it.increment(ec)) {
        const auto path = it->path();
        if (it->is_directory(ec)) {
            if (titleRootForPath(path)) {
                candidates.push_back(path);
                it.disable_recursion_pending();
            }
            continue;
        }
        if (it->is_regular_file(ec)) {
            const auto ext = lowerExtension(path);
            if (ext == L".wud" || ext == L".wux" || ext == L".wua" || ext == L".iso" || ext == L".rpx" || path.filename() == L"title.tmd") {
                candidates.push_back(path);
            }
        }
    }

    std::sort(candidates.begin(), candidates.end());
    for (const auto& candidate : candidates) {
        auto validation = validateForGame(game, candidate, GameImageSource::BundledRomsFolder);
        if (validation.valid && validation.image) {
            return validation.image;
        }
    }

    return std::nullopt;
}

std::optional<ResolvedGameImage> RomLocator::resolveFromPersistedPath(GameId game) {
    auto paths = loadPersistedPaths(config_.persistedPathsFile);
    const auto key = gameIdToConfigKey(game);
    const auto it = paths.find(key);
    if (it == paths.end()) {
        return std::nullopt;
    }

    auto validation = validateForGame(game, it->second, GameImageSource::PersistedExternalPath);
    if (validation.valid && validation.image) {
        return validation.image;
    }

    paths.erase(it);
    savePersistedPaths(config_.persistedPathsFile, paths);
    return std::nullopt;
}

void RomLocator::persist(GameId game, const std::filesystem::path& path) const {
    auto paths = loadPersistedPaths(config_.persistedPathsFile);
    paths[gameIdToConfigKey(game)] = path;
    savePersistedPaths(config_.persistedPathsFile, paths);
}

void RomLocator::clearPersisted(GameId game) const {
    auto paths = loadPersistedPaths(config_.persistedPathsFile);
    paths.erase(gameIdToConfigKey(game));
    savePersistedPaths(config_.persistedPathsFile, paths);
}

} // namespace skylanders
