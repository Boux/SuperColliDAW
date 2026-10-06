#pragma once

#include <optional>
#include <span>

namespace supercollidaw {

struct FontMetrics {
    int ascent;
    int descent;
    int unitsPerEm;
};

// The metrics ImGui's stb_truetype loader sizes a font with: hhea's ascender and descender, and head's unitsPerEm.
std::optional<FontMetrics> readFontMetrics(std::span<const unsigned char> ttf);

}
