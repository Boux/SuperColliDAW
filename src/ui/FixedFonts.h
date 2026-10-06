#pragma once

struct ImFont;

namespace supercollidaw {

// Fonts that do not change with the font setting: the icons, and the default text font for the settings popup.
struct FixedFonts {
    ImFont* icons = nullptr;
    ImFont* text = nullptr;
    float textSize = 0.f;
};

}
