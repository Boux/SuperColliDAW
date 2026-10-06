#pragma once

#include <string>

namespace supercollidaw {

constexpr char kDefaultFont[] = "DejaVu Sans Mono";

struct EditorSettings {
    static constexpr int kMinFontSizePercent = 50;
    static constexpr int kMaxFontSizePercent = 200;

    std::string font = kDefaultFont;
    int fontSizePercent = 100;

    bool operator==(const EditorSettings&) const = default;
};

}
