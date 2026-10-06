#pragma once

#include <span>
#include <string_view>

namespace supercollidaw {

struct BundledFont {
    const char* name;
    std::span<const unsigned char> data;
    // A pixel font is only sharp at whole multiples of the height its pixels were drawn at; 0 for outline fonts.
    float pixelHeight = 0.f;
};

std::span<const BundledFont> bundledFonts();
std::span<const unsigned char> iconFontData();

}
