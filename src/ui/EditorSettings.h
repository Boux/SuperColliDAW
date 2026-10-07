#pragma once

#include <array>
#include <string>

namespace supercollidaw {

constexpr char kDefaultTheme[] = "Dark";
constexpr char kDefaultFont[] = "DejaVu Sans Mono";

struct EditorSettings {
    static constexpr int kMinFontSizePercent = 50;
    static constexpr int kMaxFontSizePercent = 200;
    static constexpr std::array kTabSizes = { 2, 4, 8 };

    std::string theme = kDefaultTheme;
    std::string font = kDefaultFont;
    int fontSizePercent = 100;
    int tabSize = 4;
    bool indentWithSpaces = false;
    bool closeBrackets = true;

    bool operator==(const EditorSettings&) const = default;
};

}
