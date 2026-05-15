#include "skylanders/GraphicPackManager.h"

#include "skylanders/CemuConfigEditors.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <urlmon.h>

namespace skylanders {
namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeFile(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("failed to write " + path.string());
    }
    out << text;
}

std::string jsonEscape(std::string value) {
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

std::string jsonUnescape(std::string value) {
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

std::optional<std::string> regexStringField(const std::string& json, const char* name) {
    std::smatch match;
    const std::regex pattern(std::string("\"") + name + R"json("\s*:\s*"((?:\\.|[^"])*)")json");
    if (!std::regex_search(json, match, pattern)) {
        return std::nullopt;
    }
    return jsonUnescape(match[1].str());
}

std::optional<int> regexIntField(const std::string& json, const char* name) {
    std::smatch match;
    const std::regex pattern(std::string("\"") + name + R"json("\s*:\s*(-?\d+))json");
    if (!std::regex_search(json, match, pattern)) {
        return std::nullopt;
    }
    return std::stoi(match[1].str());
}

std::string utcNow() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_s(&tm, &time);
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::optional<std::chrono::system_clock::time_point> parseUtc(std::string value) {
    if (value.empty()) {
        return std::nullopt;
    }
    std::tm tm{};
    std::istringstream in(value);
    in >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    if (!in) {
        return std::nullopt;
    }
    return std::chrono::system_clock::from_time_t(_mkgmtime(&tm));
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

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string htmlUnescape(std::string value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value.compare(i, 5, "&amp;") == 0) {
            out.push_back('&');
            i += 4;
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

std::string resolveUrl(const std::string& href, const std::string& baseUrl) {
    if (href.rfind("http://", 0) == 0 || href.rfind("https://", 0) == 0) {
        return href;
    }
    if (href.rfind("//", 0) == 0) {
        return "https:" + href;
    }
    if (!baseUrl.empty() && href.rfind("/", 0) == 0) {
        const auto scheme = baseUrl.find("://");
        if (scheme != std::string::npos) {
            const auto slash = baseUrl.find('/', scheme + 3);
            return baseUrl.substr(0, slash == std::string::npos ? baseUrl.size() : slash) + href;
        }
    }
    if (baseUrl.empty() || baseUrl.back() == '/') {
        return baseUrl + href;
    }
    return baseUrl + "/" + href;
}

std::string fileHash(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::uint64_t hash = 1469598103934665603ull;
    char buffer[8192];
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0) {
        for (std::streamsize i = 0; i < in.gcount(); ++i) {
            hash ^= static_cast<unsigned char>(buffer[i]);
            hash *= 1099511628211ull;
        }
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

std::wstring powershellQuote(const std::filesystem::path& path) {
    auto text = path.wstring();
    std::wstring quoted = L"'";
    for (wchar_t ch : text) {
        if (ch == L'\'') {
            quoted += L"''";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted += L"'";
    return quoted;
}

bool runHiddenProcess(std::wstring commandLine, DWORD* exitCode) {
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');
    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        return false;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (exitCode) {
        *exitCode = code;
    }
    return code == 0;
}

std::filesystem::path chooseExtractedPackRoot(const std::filesystem::path& extractRoot) {
    std::error_code ec;
    const auto directGraphicPacks = extractRoot / "graphicPacks";
    if (std::filesystem::exists(directGraphicPacks, ec)) {
        return directGraphicPacks;
    }

    std::vector<std::filesystem::path> children;
    for (const auto& entry : std::filesystem::directory_iterator(extractRoot, ec)) {
        if (!ec && entry.is_directory(ec)) {
            children.push_back(entry.path());
        }
    }
    if (children.size() == 1) {
        if (std::filesystem::exists(children.front() / "graphicPacks", ec)) {
            return children.front() / "graphicPacks";
        }
        return children.front();
    }
    return extractRoot;
}

} // namespace

GraphicPackManager::GraphicPackManager(GraphicPackManagerConfig config)
    : config_(std::move(config)) {}

GraphicPackUpdateState GraphicPackManager::loadState() const {
    GraphicPackUpdateState state;
    const auto json = readFile(config_.statePath);
    if (json.empty()) {
        state.installedPackCount = static_cast<int>(scanCemuGraphicPacks(config_.cemuRoot).size());
        return state;
    }
    if (auto value = regexStringField(json, "status")) state.status = *value;
    if (auto value = regexStringField(json, "sourceUrl")) state.sourceUrl = *value;
    if (auto value = regexStringField(json, "archiveHash")) state.archiveHash = *value;
    if (auto value = regexStringField(json, "lastCheckUtc")) state.lastCheckUtc = *value;
    if (auto value = regexStringField(json, "lastInstallUtc")) state.lastInstallUtc = *value;
    if (auto value = regexStringField(json, "lastError")) state.lastError = *value;
    if (auto value = regexIntField(json, "installedPackCount")) state.installedPackCount = *value;
    return state;
}

void GraphicPackManager::saveState(const GraphicPackUpdateState& state) const {
    writeFile(config_.statePath, stateToJson(state));
}

bool GraphicPackManager::shouldAutoCheck(std::chrono::system_clock::time_point now) const {
    const auto state = loadState();
    const auto lastCheck = parseUtc(state.lastCheckUtc);
    if (!lastCheck) {
        return true;
    }
    return now - *lastCheck >= std::chrono::hours(24 * 7);
}

GraphicPackInstallResult GraphicPackManager::update(bool force) {
    if (!force && !shouldAutoCheck()) {
        const auto state = loadState();
        return {true, false, state.status, state.sourceUrl, state.archiveHash, state.lastError, state.installedPackCount};
    }

    GraphicPackUpdateState state = loadState();
    state.lastCheckUtc = utcNow();
    state.status = "checking";
    state.lastError.clear();
    saveState(state);

    try {
        std::filesystem::create_directories(config_.downloadRoot);
        const auto htmlPath = config_.downloadRoot / "cemu_graphic_packs.html";
        if (FAILED(URLDownloadToFileW(nullptr, widen(config_.indexUrl).c_str(), htmlPath.c_str(), 0, nullptr))) {
            throw std::runtime_error("failed to download Cemu graphic packs index");
        }

        const auto zipUrl = findVersion6ZipUrl(readFile(htmlPath), config_.indexUrl);
        if (!zipUrl) {
            throw std::runtime_error("could not find a version-6 graphic packs ZIP link");
        }

        const auto zipPath = config_.downloadRoot / "graphic_packs.zip";
        if (FAILED(URLDownloadToFileW(nullptr, widen(*zipUrl).c_str(), zipPath.c_str(), 0, nullptr))) {
            throw std::runtime_error("failed to download Cemu graphic packs ZIP");
        }
        return installZip(zipPath, *zipUrl);
    } catch (const std::exception& ex) {
        state.status = "error";
        state.lastError = ex.what();
        state.installedPackCount = static_cast<int>(scanCemuGraphicPacks(config_.cemuRoot).size());
        saveState(state);
        return {false, true, state.status, state.sourceUrl, state.archiveHash, state.lastError, state.installedPackCount};
    }
}

GraphicPackInstallResult GraphicPackManager::installZip(const std::filesystem::path& zipPath, const std::string& sourceUrl) {
    GraphicPackUpdateState state = loadState();
    state.lastCheckUtc = state.lastCheckUtc.empty() ? utcNow() : state.lastCheckUtc;
    state.sourceUrl = sourceUrl;
    state.archiveHash = fileHash(zipPath);
    state.status = "extracting";
    state.lastError.clear();
    saveState(state);

    try {
        const auto extractRoot = config_.downloadRoot / "extract";
        std::error_code ec;
        std::filesystem::remove_all(extractRoot, ec);
        std::filesystem::create_directories(extractRoot);

        std::wstring command =
            L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"Expand-Archive -LiteralPath " +
            powershellQuote(zipPath) +
            L" -DestinationPath " +
            powershellQuote(extractRoot) +
            L" -Force\"";
        DWORD exitCode = 1;
        if (!runHiddenProcess(command, &exitCode)) {
            throw std::runtime_error("failed to extract graphic packs ZIP with PowerShell (exit " + std::to_string(exitCode) + ")");
        }

        const auto sourceRoot = chooseExtractedPackRoot(extractRoot);
        const auto targetRoot = config_.cemuRoot / "graphicPacks";
        std::filesystem::create_directories(targetRoot);
        std::filesystem::copy(sourceRoot, targetRoot, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

        state.status = "installed";
        state.lastInstallUtc = utcNow();
        state.lastError.clear();
        state.installedPackCount = static_cast<int>(scanCemuGraphicPacks(config_.cemuRoot).size());
        saveState(state);
        return {true, true, state.status, state.sourceUrl, state.archiveHash, {}, state.installedPackCount};
    } catch (const std::exception& ex) {
        state.status = "error";
        state.lastError = ex.what();
        state.installedPackCount = static_cast<int>(scanCemuGraphicPacks(config_.cemuRoot).size());
        saveState(state);
        return {false, true, state.status, state.sourceUrl, state.archiveHash, state.lastError, state.installedPackCount};
    }
}

std::optional<std::string> GraphicPackManager::findVersion6ZipUrl(const std::string& html, const std::string& baseUrl) {
    const std::regex hrefPattern(R"regex(href\s*=\s*["']([^"']+\.zip(?:\?[^"']*)?)["'])regex", std::regex::icase);
    std::optional<std::string> fallback;
    for (auto it = std::sregex_iterator(html.begin(), html.end(), hrefPattern);
         it != std::sregex_iterator(); ++it) {
        auto href = htmlUnescape((*it)[1].str());
        const auto lower = toLower(href);
        if (lower.find("graphic") == std::string::npos) {
            continue;
        }
        const auto resolved = resolveUrl(href, baseUrl);
        if (!fallback) {
            fallback = resolved;
        }
        if (lower.find("v6") != std::string::npos ||
            lower.find("version-6") != std::string::npos ||
            lower.find("version_6") != std::string::npos ||
            lower.find("graphicpacks") != std::string::npos ||
            lower.find("graphic_packs") != std::string::npos) {
            return resolved;
        }
    }
    return fallback;
}

std::string GraphicPackManager::stateToJson(const GraphicPackUpdateState& state) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"status\": \"" << jsonEscape(state.status) << "\",\n";
    json << "  \"sourceUrl\": \"" << jsonEscape(state.sourceUrl) << "\",\n";
    json << "  \"archiveHash\": \"" << jsonEscape(state.archiveHash) << "\",\n";
    json << "  \"lastCheckUtc\": \"" << jsonEscape(state.lastCheckUtc) << "\",\n";
    json << "  \"lastInstallUtc\": \"" << jsonEscape(state.lastInstallUtc) << "\",\n";
    json << "  \"lastError\": \"" << jsonEscape(state.lastError) << "\",\n";
    json << "  \"installedPackCount\": " << state.installedPackCount << "\n";
    json << "}\n";
    return json.str();
}

} // namespace skylanders
