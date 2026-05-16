#include "skylanders/CemuLocator.h"

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>

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
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
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

bool filenameEqualsCemuExe(const std::filesystem::path& path) {
    auto filename = path.filename().wstring();
    std::transform(filename.begin(), filename.end(), filename.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(std::towlower(ch));
    });
    return filename == L"cemu.exe";
}

} // namespace

SetupConfig SetupConfig::load(const std::filesystem::path& path) {
    SetupConfig config;
    const auto text = readFile(path);
    if (text.empty()) {
        return config;
    }

    const std::regex pathPattern(R"json("externalCemuExePath"\s*:\s*"((?:\\.|[^"])*)")json");
    std::smatch match;
    if (std::regex_search(text, match, pathPattern)) {
        config.externalCemuExePath = widen(unescapeJson(match[1].str()));
    }
    const std::regex fullscreenPattern(R"json("fullscreen"\s*:\s*(true|false))json");
    if (std::regex_search(text, match, fullscreenPattern)) {
        config.fullscreen = match[1].str() == "true";
    }
    const std::regex cemuBackendPattern(R"json("cemuBackendMode"\s*:\s*"((?:\\.|[^"])*)")json");
    if (std::regex_search(text, match, cemuBackendPattern)) {
        config.cemuBackendMode = cemuBackendModeFromString(unescapeJson(match[1].str()));
    }
    const std::regex overlayBackendPattern(R"json("overlayBackendMode"\s*:\s*"((?:\\.|[^"])*)")json");
    if (std::regex_search(text, match, overlayBackendPattern)) {
        config.overlayBackendMode = overlayBackendModeFromString(unescapeJson(match[1].str()));
    }
    return config;
}

void SetupConfig::save(const std::filesystem::path& path) const {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return;
    }

    out << "{\n";
    bool wroteField = false;
    if (externalCemuExePath) {
        out << "  \"externalCemuExePath\": \"" << escapeJson(narrow(externalCemuExePath->wstring())) << "\"";
        wroteField = true;
    }
    if (fullscreen) {
        if (wroteField) {
            out << ",\n";
        }
        out << "  \"fullscreen\": " << (*fullscreen ? "true" : "false");
        wroteField = true;
    }
    if (cemuBackendMode) {
        if (wroteField) {
            out << ",\n";
        }
        out << "  \"cemuBackendMode\": \"" << cemuBackendModeToString(*cemuBackendMode) << "\"";
        wroteField = true;
    }
    if (overlayBackendMode) {
        if (wroteField) {
            out << ",\n";
        }
        out << "  \"overlayBackendMode\": \"" << overlayBackendModeToString(*overlayBackendMode) << "\"";
        wroteField = true;
    }
    if (wroteField) {
        out << "\n";
    }
    out << "}\n";
}

CemuLocator::CemuLocator(CemuLocatorConfig config)
    : config_(std::move(config)) {}

CemuValidationResult CemuLocator::validateCemuExe(const std::filesystem::path& path) const {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
        return {false, "Cemu.exe was not found"};
    }
    if (!filenameEqualsCemuExe(path)) {
        return {false, "selected file is not Cemu.exe"};
    }
    return {true, {}};
}

std::optional<CemuResolution> CemuLocator::validateAs(const std::filesystem::path& path, CemuSource source) const {
    const auto validation = validateCemuExe(path);
    if (!validation.valid) {
        return std::nullopt;
    }
    return CemuResolution{path.parent_path(), path, source};
}

std::optional<CemuResolution> CemuLocator::resolve() {
    const auto exeDir = config_.exePath.parent_path();
    for (const auto& bundled : {
        exeDir / "cemu" / "Cemu.exe",
    }) {
        if (auto resolution = validateAs(bundled, CemuSource::BundledSubmodule)) {
            return resolution;
        }
    }

    auto setup = SetupConfig::load(config_.setupConfigFile);
    if (setup.externalCemuExePath) {
        if (auto resolution = validateAs(*setup.externalCemuExePath, CemuSource::PersistedExternal)) {
            return resolution;
        }
        clearPersisted();
    }

    for (const auto& portable : {
        config_.projectRoot / "cemu" / "Cemu.exe",
        exeDir / "Cemu.exe",
    }) {
        if (auto resolution = validateAs(portable, CemuSource::PortableLocal)) {
            return resolution;
        }
    }

    if (auto resolution = validateAs(config_.legacyCemuExe, CemuSource::LegacyFallback)) {
        return resolution;
    }

    return std::nullopt;
}

std::optional<CemuResolution> CemuLocator::promptAndPersist(void* ownerWindow) {
    std::wstring buffer(MAX_PATH * 4, L'\0');

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = static_cast<HWND>(ownerWindow);
    dialog.lpstrFilter = L"Cemu executable\0Cemu.exe\0Executable files\0*.exe\0All files\0*.*\0\0";
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrTitle = L"Select Cemu.exe";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&dialog)) {
        return std::nullopt;
    }

    const std::filesystem::path selected(buffer.c_str());
    auto resolution = validateAs(selected, CemuSource::PickedExternal);
    if (!resolution) {
        MessageBoxA(static_cast<HWND>(ownerWindow), validateCemuExe(selected).reason.c_str(), "Skylanders Engine", MB_OK | MB_ICONERROR);
        return std::nullopt;
    }

    persistExternal(selected);
    return resolution;
}

void CemuLocator::clearPersisted() const {
    auto config = SetupConfig::load(config_.setupConfigFile);
    config.externalCemuExePath.reset();
    config.save(config_.setupConfigFile);
}

void CemuLocator::persistExternal(const std::filesystem::path& cemuExe) const {
    auto config = SetupConfig::load(config_.setupConfigFile);
    config.externalCemuExePath = cemuExe;
    config.save(config_.setupConfigFile);
}

std::string CemuLocator::sourceToString(CemuSource source) {
    switch (source) {
    case CemuSource::BundledSubmodule: return "bundled_submodule";
    case CemuSource::PersistedExternal: return "persisted_external";
    case CemuSource::PortableLocal: return "portable_local";
    case CemuSource::LegacyFallback: return "legacy_fallback";
    case CemuSource::PickedExternal: return "picked_external";
    }
    return "unknown";
}

} // namespace skylanders
