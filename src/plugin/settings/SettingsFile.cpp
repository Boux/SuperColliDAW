#include "SettingsFile.h"

#include <algorithm>
#include <charconv>
#include <ranges>

namespace supercollidaw {

namespace {

constexpr std::string_view kFontKey = "font";
constexpr std::string_view kFontSizeKey = "font_size_percent";
constexpr std::string_view kBlank = " \t\r";

std::string_view trimmed(std::string_view text) {
    const size_t first = text.find_first_not_of(kBlank);
    return first == std::string_view::npos ? std::string_view() : text.substr(first, text.find_last_not_of(kBlank) - first + 1);
}

int parsePercent(std::string_view text, int fallback) {
    int percent = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), percent);
    if (error != std::errc() || end != text.data() + text.size())
        return fallback;
    return std::clamp(percent, EditorSettings::kMinFontSizePercent, EditorSettings::kMaxFontSizePercent);
}

void applyEntry(EditorSettings& settings, std::string_view key, std::string_view value) {
    if (key == kFontKey)
        settings.font = value;
    if (key == kFontSizeKey)
        settings.fontSizePercent = parsePercent(value, settings.fontSizePercent);
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
    return std::string(kFontKey) + " = " + settings.font + "\n" + std::string(kFontSizeKey) + " = " + std::to_string(settings.fontSizePercent) + "\n";
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
