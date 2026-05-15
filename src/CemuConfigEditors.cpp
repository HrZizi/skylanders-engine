#include "skylanders/CemuConfigEditors.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace skylanders {
namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("failed to read " + path.string());
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

std::string trim(std::string value) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string normalizeSlashes(std::filesystem::path path) {
    return path.generic_string();
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

bool containsNeedle(const std::string& normalizedHaystack, std::string_view needle) {
    return normalizedHaystack.find(normalizeText(std::string(needle))) != std::string::npos;
}

std::optional<std::string> parseIniValue(const std::string& text, std::string_view key) {
    const std::regex pattern(
        "^\\s*" + std::string(key) + R"(\s*=\s*(.*?)\s*(?:#.*)?$)",
        std::regex::icase | std::regex::multiline);
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        return std::nullopt;
    }

    auto value = trim(match[1].str());
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

std::vector<GraphicPackCandidate::Preset> parsePresets(const std::string& text) {
    std::vector<GraphicPackCandidate::Preset> presets;
    const std::regex presetBlock(R"(\[Preset\]([\s\S]*?)(?=\n\s*\[|$))", std::regex::icase);
    for (auto it = std::sregex_iterator(text.begin(), text.end(), presetBlock);
         it != std::sregex_iterator(); ++it) {
        if (auto name = parseIniValue((*it)[1].str(), "name")) {
            presets.push_back(GraphicPackCandidate::Preset{
                parseIniValue((*it)[1].str(), "category").value_or(""),
                *name,
            });
        }
    }
    return presets;
}

std::vector<std::uint64_t> parseTitleIds(const std::string& text) {
    std::vector<std::uint64_t> ids;
    const auto raw = parseIniValue(text, "titleIds");
    if (!raw) {
        return ids;
    }

    std::stringstream stream(*raw);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim(token);
        if (token.empty()) {
            continue;
        }
        try {
            ids.push_back(std::stoull(token, nullptr, 16));
        } catch (const std::exception&) {
        }
    }
    return ids;
}

std::string xmlEscape(std::string value) {
    std::string out;
    out.reserve(value.size());
    for (const char c : value) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default: out += c; break;
        }
    }
    return out;
}

bool replaceTag(std::string& xml, const std::string& tag, const std::string& replacement) {
    const std::regex fullTag("<" + tag + R"((?:\s[^>]*)?>[\s\S]*?</)" + tag + ">", std::regex::icase);
    if (std::regex_search(xml, fullTag)) {
        xml = std::regex_replace(xml, fullTag, replacement, std::regex_constants::format_first_only);
        return true;
    }

    const std::regex selfClosing("<" + tag + R"((?:\s[^>]*)?\s*/>)", std::regex::icase);
    if (std::regex_search(xml, selfClosing)) {
        xml = std::regex_replace(xml, selfClosing, replacement, std::regex_constants::format_first_only);
        return true;
    }

    return false;
}

void insertBeforeClosing(std::string& xml, const std::string& parent, const std::string& childXml) {
    const auto closing = "</" + parent + ">";
    const auto pos = xml.rfind(closing);
    if (pos == std::string::npos) {
        throw std::runtime_error("settings.xml is missing closing tag: " + closing);
    }

    xml.insert(pos, childXml);
}

void ensureTopLevelChild(std::string& xml, const std::string& tag) {
    if (std::regex_search(xml, std::regex("<" + tag + R"((?:\s|>|/))", std::regex::icase))) {
        return;
    }

    insertBeforeClosing(xml, "content", "  <" + tag + "/>\n");
}

void upsertTopLevelValue(std::string& xml, const std::string& tag, const std::string& value) {
    const auto replacement = "  <" + tag + ">" + xmlEscape(value) + "</" + tag + ">";
    if (!replaceTag(xml, tag, replacement)) {
        insertBeforeClosing(xml, "content", replacement + "\n");
    }
}

void upsertNestedValue(std::string& xml, const std::string& parent, const std::string& tag, const std::string& value) {
    ensureTopLevelChild(xml, parent);

    const std::regex parentPattern("(<" + parent + R"((?:\s[^>]*)?>)([\s\S]*?)(</)" + parent + ">)", std::regex::icase);
    std::smatch match;
    if (!std::regex_search(xml, match, parentPattern)) {
        throw std::runtime_error("settings.xml parent tag was not writable: " + parent);
    }

    std::string body = match[2].str();
    const auto replacement = "    <" + tag + ">" + xmlEscape(value) + "</" + tag + ">";
    if (!replaceTag(body, tag, replacement)) {
        body += replacement + "\n";
    }

    xml.replace(match.position(0), match.length(0), match[1].str() + body + match[3].str());
}

std::string makeGraphicPackBlock(const std::map<std::string, CemuSettingsXml::GraphicPackSelection>& graphicPacks) {
    std::ostringstream xml;
    xml << "  <GraphicPack>\n";
    for (const auto& [filename, selection] : graphicPacks) {
        xml << "    <Entry filename=\"" << xmlEscape(filename) << "\"";
        if (selection.disabled) {
            xml << " disabled=\"true\"";
        }
        xml << ">\n";
        for (const auto& [category, preset] : selection.presets) {
            xml << "      <Preset>\n";
            if (!category.empty()) {
                xml << "        <category>" << xmlEscape(category) << "</category>\n";
            }
            xml << "        <preset>" << xmlEscape(preset) << "</preset>\n";
            xml << "      </Preset>\n";
        }
        xml << "    </Entry>\n";
    }
    xml << "  </GraphicPack>";
    return xml.str();
}

} // namespace

CemuSettingsXml::CemuSettingsXml(std::filesystem::path path)
    : path_(std::move(path)) {}

void CemuSettingsXml::loadOrCreate() {
    values_.clear();
    graphicPacks_.clear();

    if (!std::filesystem::exists(path_)) {
        rawXml_ = defaultSettingsXml();
        values_["mlc_path"] = "";
        values_["fullscreen"] = "true";
        values_["fullscreen_menubar"] = "false";
        values_["gp_download"] = "true";
        values_["Graphic/api"] = "1";
        values_["Graphic/VSync"] = "0";
        values_["Graphic/GX2DrawdoneSync"] = "true";
        values_["Graphic/UpscaleFilter"] = "2";
        values_["Graphic/DownscaleFilter"] = "0";
        values_["Graphic/FullscreenScaling"] = "0";
        values_["Graphic/AsyncCompile"] = "true";
        values_["Graphic/vkAccurateBarriers"] = "true";
        values_["Audio/api"] = "2";
        values_["Audio/delay"] = "2";
        values_["Audio/TVDevice"] = "default";
        values_["EmulatedUsbDevices/EmulateSkylanderPortal"] = "true";
        return;
    }

    rawXml_ = readFile(path_);

    const std::vector<std::string> scalarTags = {
        "mlc_path", "fullscreen", "fullscreen_menubar", "gp_download",
        "Graphic/api", "Graphic/VSync", "Graphic/GX2DrawdoneSync",
        "Graphic/UpscaleFilter", "Graphic/DownscaleFilter",
        "Graphic/FullscreenScaling", "Graphic/AsyncCompile",
        "Graphic/vkAccurateBarriers",
        "Audio/api", "Audio/delay", "Audio/TVDevice",
        "EmulatedUsbDevices/EmulateSkylanderPortal",
    };

    for (const auto& path : scalarTags) {
        const auto slash = path.find('/');
        std::string pattern;
        if (slash == std::string::npos) {
            pattern = "<" + path + R"(>\s*([^<]*)\s*</)" + path + ">";
        } else {
            const auto parent = path.substr(0, slash);
            const auto child = path.substr(slash + 1);
            pattern = "<" + parent + R"([^>]*>[\s\S]*?<)" + child + R"(>\s*([^<]*)\s*</)" + child + ">";
        }

        std::smatch match;
        if (std::regex_search(rawXml_, match, std::regex(pattern, std::regex::icase))) {
            values_[path] = trim(match[1].str());
        }
    }

    const std::regex entryPattern(R"regex(<Entry\s+filename="([^"]+)"([^>]*)>([\s\S]*?)</Entry>)regex", std::regex::icase);
    for (auto it = std::sregex_iterator(rawXml_.begin(), rawXml_.end(), entryPattern);
         it != std::sregex_iterator(); ++it) {
        const auto filename = (*it)[1].str();
        const auto attrs = (*it)[2].str();
        const auto body = (*it)[3].str();
        auto& entry = graphicPacks_[filename];
        entry.disabled = attrs.find("disabled=\"true\"") != std::string::npos;

        const std::regex presetPattern(R"regex(<Preset([^>]*)>([\s\S]*?)</Preset>|<Preset([^>]*)\s*/>)regex", std::regex::icase);
        for (auto preset = std::sregex_iterator(body.begin(), body.end(), presetPattern);
             preset != std::sregex_iterator(); ++preset) {
            const std::string attrs = (*preset)[1].matched ? (*preset)[1].str() : (*preset)[3].str();
            const std::string presetBody = (*preset)[2].matched ? (*preset)[2].str() : "";

            std::string category;
            std::string presetName;
            std::smatch valueMatch;
            if (std::regex_search(attrs, valueMatch, std::regex(R"regex(category="([^"]*)")regex", std::regex::icase))) {
                category = valueMatch[1].str();
            }
            if (std::regex_search(attrs, valueMatch, std::regex(R"regex(preset="([^"]*)")regex", std::regex::icase))) {
                presetName = valueMatch[1].str();
            }
            if (category.empty() && std::regex_search(presetBody, valueMatch, std::regex(R"regex(<category>\s*([^<]*)\s*</category>)regex", std::regex::icase))) {
                category = trim(valueMatch[1].str());
            }
            if (presetName.empty() && std::regex_search(presetBody, valueMatch, std::regex(R"regex(<preset>\s*([^<]*)\s*</preset>)regex", std::regex::icase))) {
                presetName = trim(valueMatch[1].str());
            }
            entry.presets[category] = presetName;
        }
    }
}

void CemuSettingsXml::save() const {
    std::string xml = rawXml_.empty() ? defaultSettingsXml() : rawXml_;
    if (!std::regex_search(xml, std::regex(R"(<content(?:\s[^>]*)?>)", std::regex::icase))) {
        xml = defaultSettingsXml();
    }

    for (const auto& [path, value] : values_) {
        const auto slash = path.find('/');
        if (slash == std::string::npos) {
            upsertTopLevelValue(xml, path, value);
        } else {
            upsertNestedValue(xml, path.substr(0, slash), path.substr(slash + 1), value);
        }
    }

    const auto graphicPackBlock = makeGraphicPackBlock(graphicPacks_);
    if (!replaceTag(xml, "GraphicPack", graphicPackBlock)) {
        insertBeforeClosing(xml, "content", graphicPackBlock + "\n");
    }

    writeFile(path_, xml);
}

void CemuSettingsXml::setContentValue(const std::vector<std::string>& path, const std::string& value) {
    if (path.empty() || path.front() != "content") {
        throw std::invalid_argument("Cemu settings path must start with content");
    }

    std::string key;
    for (size_t i = 1; i < path.size(); ++i) {
        if (!key.empty()) {
            key += '/';
        }
        key += path[i];
    }
    values_[key] = value;
}

std::optional<std::string> CemuSettingsXml::contentValue(const std::vector<std::string>& path) const {
    if (path.empty() || path.front() != "content") {
        return std::nullopt;
    }

    std::string key;
    for (size_t i = 1; i < path.size(); ++i) {
        if (!key.empty()) {
            key += '/';
        }
        key += path[i];
    }
    if (auto it = values_.find(key); it != values_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void CemuSettingsXml::enableGraphicPack(const std::filesystem::path& normalizedPackPath) {
    graphicPacks_[normalizeSlashes(normalizedPackPath)].disabled = false;
}

void CemuSettingsXml::setGraphicPackPreset(
    const std::filesystem::path& normalizedPackPath,
    const std::string& category,
    const std::string& preset) {
    auto& entry = graphicPacks_[normalizeSlashes(normalizedPackPath)];
    entry.disabled = false;
    entry.presets[category] = preset;
}

std::string CemuSettingsXml::defaultSettingsXml() {
    return
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<content>\n"
        "  <mlc_path></mlc_path>\n"
        "  <permanent_storage>true</permanent_storage>\n"
        "  <fullscreen>true</fullscreen>\n"
        "  <fullscreen_menubar>false</fullscreen_menubar>\n"
        "  <gp_download>true</gp_download>\n"
        "  <GamePaths/>\n"
        "  <GameCache/>\n"
        "  <GraphicPack/>\n"
        "  <Graphic>\n"
        "    <api>1</api>\n"
        "    <device>00000000000000000000000000000000</device>\n"
        "    <vkDevice>00000000000000000000000000000000</vkDevice>\n"
        "    <VSync>0</VSync>\n"
        "    <GX2DrawdoneSync>true</GX2DrawdoneSync>\n"
        "    <UpscaleFilter>2</UpscaleFilter>\n"
        "    <DownscaleFilter>0</DownscaleFilter>\n"
        "    <FullscreenScaling>0</FullscreenScaling>\n"
        "    <AsyncCompile>true</AsyncCompile>\n"
        "    <vkAccurateBarriers>true</vkAccurateBarriers>\n"
        "  </Graphic>\n"
        "  <Input>\n"
        "    <DSUC host=\"127.0.0.1\" port=\"26760\"/>\n"
        "  </Input>\n"
        "  <Audio>\n"
        "    <api>2</api>\n"
        "    <delay>2</delay>\n"
        "    <TVDevice>default</TVDevice>\n"
        "  </Audio>\n"
        "  <EmulatedUsbDevices>\n"
        "    <EmulateSkylanderPortal>true</EmulateSkylanderPortal>\n"
        "    <EmulateInfinityBase>false</EmulateInfinityBase>\n"
        "    <EmulateDimensionsToypad>false</EmulateDimensionsToypad>\n"
        "  </EmulatedUsbDevices>\n"
        "</content>\n";
}

IniDocument::IniDocument(std::filesystem::path path)
    : path_(std::move(path)) {}

void IniDocument::loadIfExists() {
    sections_.clear();
    leadingComments_.clear();
    if (!std::filesystem::exists(path_)) {
        return;
    }

    std::ifstream in(path_);
    if (!in) {
        throw std::runtime_error("failed to read " + path_.string());
    }

    std::string section;
    std::string line;
    while (std::getline(in, line)) {
        const auto stripped = trim(line);
        if (stripped.empty()) {
            continue;
        }
        if (stripped.front() == '#' || stripped.front() == ';') {
            if (section.empty()) {
                leadingComments_.push_back(stripped);
            }
            continue;
        }
        if (stripped.front() == '[' && stripped.back() == ']') {
            section = stripped.substr(1, stripped.size() - 2);
            continue;
        }

        const auto eq = stripped.find('=');
        if (eq == std::string::npos || section.empty()) {
            continue;
        }
        sections_[section][trim(stripped.substr(0, eq))] = trim(stripped.substr(eq + 1));
    }
}

void IniDocument::set(const std::string& section, const std::string& key, const std::string& value) {
    sections_[section][key] = value;
}

void IniDocument::erase(const std::string& section, const std::string& key) {
    if (auto sectionIt = sections_.find(section); sectionIt != sections_.end()) {
        sectionIt->second.erase(key);
    }
}

void IniDocument::save() const {
    std::filesystem::create_directories(path_.parent_path());
    std::ofstream out(path_, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("failed to write " + path_.string());
    }

    for (const auto& comment : leadingComments_) {
        out << comment << '\n';
    }
    if (!leadingComments_.empty()) {
        out << '\n';
    }

    for (const auto& [section, keys] : sections_) {
        out << '[' << section << "]\n";
        for (const auto& [key, value] : keys) {
            out << key << " = " << value << '\n';
        }
        out << '\n';
    }
}

std::string titleIdToHex(std::uint64_t titleId) {
    std::ostringstream out;
    out << std::hex << std::nouppercase << std::setw(16) << std::setfill('0') << titleId;
    return out.str();
}

std::vector<GraphicPackCandidate> scanCemuGraphicPacks(const std::filesystem::path& cemuRoot) {
    const auto graphicPacksRoot = cemuRoot / "graphicPacks";
    std::vector<GraphicPackCandidate> packs;

    std::error_code ec;
    if (!std::filesystem::exists(graphicPacksRoot, ec)) {
        return packs;
    }

    for (std::filesystem::recursive_directory_iterator it(graphicPacksRoot, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (!it->is_directory(ec)) {
            continue;
        }

        const auto rulesPath = it->path() / "rules.txt";
        if (!std::filesystem::exists(rulesPath, ec)) {
            continue;
        }

        GraphicPackCandidate candidate;
        candidate.rulesPath = rulesPath.lexically_normal();
        candidate.configPath = candidate.rulesPath.lexically_relative(cemuRoot).lexically_normal();
        candidate.folderName = it->path().filename().string();

        try {
            const auto text = readFile(rulesPath);
            candidate.definitionName = parseIniValue(text, "name").value_or(candidate.folderName);
            candidate.virtualPath = parseIniValue(text, "path").value_or("");
            candidate.description = parseIniValue(text, "description").value_or("");
            candidate.titleIds = parseTitleIds(text);
            candidate.presets = parsePresets(text);
            for (const auto& preset : candidate.presets) {
                candidate.presetNames.push_back(preset.name);
            }
        } catch (const std::exception&) {
            continue;
        }

        packs.push_back(std::move(candidate));
        it.disable_recursion_pending();
    }

    return packs;
}

std::optional<GraphicPackCandidate> findBestGraphicPack(
    const std::filesystem::path& cemuRoot,
    std::span<const std::string_view> gameNeedles,
    std::span<const std::string_view> requiredNeedles,
    std::span<const std::string_view> optionalNeedles,
    std::optional<std::uint64_t> titleId) {
    auto packs = scanCemuGraphicPacks(cemuRoot);
    std::optional<GraphicPackCandidate> best;

    for (auto& pack : packs) {
        const auto haystack = normalizeText(
            pack.folderName + " " + pack.definitionName + " " + pack.virtualPath + " " + pack.description);

        bool matchesGame = gameNeedles.empty();
        int score = 0;
        for (const auto needle : gameNeedles) {
            if (containsNeedle(haystack, needle)) {
                matchesGame = true;
                score += 20;
            }
        }
        if (!matchesGame) {
            continue;
        }

        bool hasRequired = true;
        for (const auto needle : requiredNeedles) {
            if (!containsNeedle(haystack, needle)) {
                hasRequired = false;
                break;
            }
            score += 30;
        }
        if (!hasRequired) {
            continue;
        }

        for (const auto needle : optionalNeedles) {
            if (containsNeedle(haystack, needle)) {
                score += 8;
            }
        }

        if (titleId && std::find(pack.titleIds.begin(), pack.titleIds.end(), *titleId) != pack.titleIds.end()) {
            score += 75;
        }

        for (const auto& preset : pack.presetNames) {
            const auto normalizedPreset = normalizeText(preset);
            for (const auto needle : requiredNeedles) {
                if (containsNeedle(normalizedPreset, needle)) {
                    score += 10;
                }
            }
            for (const auto needle : optionalNeedles) {
                if (containsNeedle(normalizedPreset, needle)) {
                    score += 4;
                }
            }
        }

        pack.score = score;
        if (!best || pack.score > best->score) {
            best = pack;
        }
    }

    return best;
}

std::optional<std::string> findBestPreset(
    const GraphicPackCandidate& pack,
    std::span<const std::string_view> requiredNeedles,
    std::span<const std::string_view> optionalNeedles) {
    if (auto preset = findBestPresetSelection(pack, requiredNeedles, optionalNeedles)) {
        return preset->name;
    }
    return std::nullopt;
}

std::optional<GraphicPackCandidate::Preset> findBestPresetSelection(
    const GraphicPackCandidate& pack,
    std::span<const std::string_view> requiredNeedles,
    std::span<const std::string_view> optionalNeedles) {
    std::optional<GraphicPackCandidate::Preset> bestPreset;
    int bestScore = -1;

    for (const auto& preset : pack.presets) {
        const auto haystack = normalizeText(preset.category + " " + preset.name);
        int score = 0;
        bool hasRequired = true;
        for (const auto needle : requiredNeedles) {
            if (!containsNeedle(haystack, needle)) {
                hasRequired = false;
                break;
            }
            score += 30;
        }
        if (!hasRequired) {
            continue;
        }

        for (const auto needle : optionalNeedles) {
            if (containsNeedle(haystack, needle)) {
                score += 8;
            }
        }

        if (score > bestScore) {
            bestScore = score;
            bestPreset = preset;
        }
    }

    return bestPreset;
}

} // namespace skylanders
