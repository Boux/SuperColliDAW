#include "Fonts.h"

#include "EditorSettings.h"

extern const unsigned char cozette_ttf[];
extern const unsigned long cozette_ttf_size;
extern const unsigned char dejavu_sans_mono_ttf[];
extern const unsigned long dejavu_sans_mono_ttf_size;
extern const unsigned char ibm_plex_mono_ttf[];
extern const unsigned long ibm_plex_mono_ttf_size;
extern const unsigned char jetbrains_mono_ttf[];
extern const unsigned long jetbrains_mono_ttf_size;
extern const unsigned char lucide_ttf[];
extern const unsigned long lucide_ttf_size;
extern const unsigned char source_code_pro_ttf[];
extern const unsigned long source_code_pro_ttf_size;

namespace supercollidaw {

namespace {

constexpr float kCozettePixelHeight = 13.f;

}

std::span<const BundledFont> bundledFonts() {
    static const BundledFont fonts[] = {
        { kDefaultFont, { dejavu_sans_mono_ttf, dejavu_sans_mono_ttf_size } },
        { "JetBrains Mono", { jetbrains_mono_ttf, jetbrains_mono_ttf_size } },
        { "Source Code Pro", { source_code_pro_ttf, source_code_pro_ttf_size } },
        { "IBM Plex Mono", { ibm_plex_mono_ttf, ibm_plex_mono_ttf_size } },
        { "Cozette", { cozette_ttf, cozette_ttf_size }, kCozettePixelHeight },
    };
    return fonts;
}

std::span<const unsigned char> iconFontData() { return { lucide_ttf, lucide_ttf_size }; }

}
