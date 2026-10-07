#include "SettingsFile.h"

#include <algorithm>
#include <charconv>
#include <numeric>
#include <ranges>

namespace supercollidaw {

namespace {

constexpr std::string_view kBlank = " \t\r";

std::string_view trimmed(std::string_view text) {
    const size_t first = text.find_first_not_of(kBlank);
    return first == std::string_view::npos ? std::string_view() : text.substr(first, text.find_last_not_of(kBlank) - first + 1);
}

std::optional<int> parseInt(std::string_view text) {
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size())
        return std::nullopt;
    return value;
}

int parsePercent(std::string_view text, int fallback) {
    const std::optional<int> percent = parseInt(text);
    return percent ? std::clamp(*percent, EditorSettings::kMinFontSizePercent, EditorSettings::kMaxFontSizePercent) : fallback;
}

int parseTabSize(std::string_view text, int fallback) {
    const std::optional<int> size = parseInt(text);
    return size && std::ranges::find(EditorSettings::kTabSizes, *size) != EditorSettings::kTabSizes.end() ? *size : fallback;
}

bool parseFlag(std::string_view text, bool fallback) {
    if (text == "true")
        return true;
    if (text == "false")
        return false;
    return fallback;
}

std::string formatFlag(bool flag) { return flag ? "true" : "false"; }

struct SettingEntry {
    std::string_view key;
    void (*read)(EditorSettings& settings, std::string_view value);
    std::string (*write)(const EditorSettings& settings);
};

constexpr SettingEntry kEntries[] = {
    { "theme", [](EditorSettings& settings, std::string_view value) { settings.theme = value; }, [](const EditorSettings& settings) { return settings.theme; } },
    { "font", [](EditorSettings& settings, std::string_view value) { settings.font = value; }, [](const EditorSettings& settings) { return settings.font; } },
    { "font_size_percent", [](EditorSettings& settings, std::string_view value) { settings.fontSizePercent = parsePercent(value, settings.fontSizePercent); }, [](const EditorSettings& settings) { return std::to_string(settings.fontSizePercent); } },
    { "tab_size", [](EditorSettings& settings, std::string_view value) { settings.tabSize = parseTabSize(value, settings.tabSize); }, [](const EditorSettings& settings) { return std::to_string(settings.tabSize); } },
    { "indent_with_spaces", [](EditorSettings& settings, std::string_view value) { settings.indentWithSpaces = parseFlag(value, settings.indentWithSpaces); }, [](const EditorSettings& settings) { return formatFlag(settings.indentWithSpaces); } },
    { "close_brackets", [](EditorSettings& settings, std::string_view value) { settings.closeBrackets = parseFlag(value, settings.closeBrackets); }, [](const EditorSettings& settings) { return formatFlag(settings.closeBrackets); } },
};

void applyEntry(EditorSettings& settings, std::string_view key, std::string_view value) {
    const auto entry = std::ranges::find(kEntries, key, &SettingEntry::key);
    if (entry != std::ranges::end(kEntries))
        entry->read(settings, value);
}

}

EditorSettings parseSettings(std::string_view text) {
    EditorSettings settings;
    for (const auto line : text | std::views::split('\n')) {
        const std::string_view entry(line.begin(), line.end());
        const size_t equals = entry.find('=');
        if (equals != std::string_view::npos)
            applyEntry(settings, trimmed(entry.substr(0, equals)), trimmed(entry.substr(equals + 1)));
    }
    return settings;
}

std::string formatSettings(const EditorSettings& settings) {
    return std::accumulate(std::begin(kEntries), std::end(kEntries), std::string(), [&](std::string text, const SettingEntry& entry) { return std::move(text) + std::string(entry.key) + " = " + entry.write(settings) + "\n"; });
}

EditorSettings SettingsFile::load() {
    const std::optional<std::string> text = mFile.read();
    if (text)
        return parseSettings(*text);
    // Without this, a file that does not exist yet would count as changed on every poll.
    mFile.markChangeSeen();
    return {};
}

std::optional<EditorSettings> SettingsFile::reloadIfChanged() {
    if (!mFile.changedSinceRead())
        return std::nullopt;
    return load();
}

bool SettingsFile::save(const EditorSettings& settings) {
    std::error_code ec;
    std::filesystem::create_directories(path().parent_path(), ec);
    return mFile.write(formatSettings(settings));
}

}
