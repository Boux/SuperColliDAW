#include "Gruvbox.h"

#include "ColorScheme.h"

namespace supercollidaw {

namespace {

using Color = TextEditor::Color;

struct GruvboxColors {
    uint32_t windowBg, bg0, bg1, bg2, bg3, bg4;
    uint32_t fg1, fg4, gray;
    uint32_t red, green, yellow, blue, purple, aqua, orange;
};

// From morhetz/gruvbox colors/gruvbox.vim; the window around the code takes the hard dark and the soft light background.
constexpr GruvboxColors kDark = {
    .windowBg = 0x1d2021, .bg0 = 0x282828, .bg1 = 0x3c3836, .bg2 = 0x504945, .bg3 = 0x665c54, .bg4 = 0x7c6f64,
    .fg1 = 0xebdbb2, .fg4 = 0xa89984, .gray = 0x928374,
    .red = 0xfb4934, .green = 0xb8bb26, .yellow = 0xfabd2f, .blue = 0x83a598, .purple = 0xd3869b, .aqua = 0x8ec07c, .orange = 0xfe8019,
};
constexpr GruvboxColors kLight = {
    .windowBg = 0xf2e5bc, .bg0 = 0xfbf1c7, .bg1 = 0xebdbb2, .bg2 = 0xd5c4a1, .bg3 = 0xbdae93, .bg4 = 0xa89984,
    .fg1 = 0x3c3836, .fg4 = 0x7c6f64, .gray = 0x928374,
    .red = 0x9d0006, .green = 0x79740e, .yellow = 0xb57614, .blue = 0x076678, .purple = 0x8f3f71, .aqua = 0x427b58, .orange = 0xaf3a03,
};
// gruvbox.vim's rainbow brackets, the same in both variants; its red is left out because red marks an unmatched bracket.
constexpr uint32_t kBracketLevels[] = { 0x458588, 0xb16286, 0xd65d0e };

WidgetColors widgetColors(const GruvboxColors& c) {
    return { .windowBg = c.windowBg, .bg1 = c.bg1, .bg2 = c.bg2, .bg3 = c.bg3, .bg4 = c.bg4, .text = c.fg1, .textDisabled = c.gray, .accent = c.blue, .accentActive = c.aqua, .highlight = c.yellow, .highlightActive = c.orange };
}

// Token colors follow scnvim's syntax groups as gruvbox.vim colors them: class names are Identifier (blue), symbols Constant (purple), operators and delimiters Special (orange).
TextEditor::Palette gruvboxPalette(const GruvboxColors& c) {
    return editorPalette({
        { Color::text, c.fg1 },
        { Color::keyword, c.red },
        { Color::declaration, c.blue },
        { Color::number, c.purple },
        { Color::string, c.green },
        { Color::punctuation, c.orange },
        { Color::preprocessor, c.aqua },
        { Color::identifier, c.fg1 },
        { Color::knownIdentifier, c.purple },
        { Color::comment, c.gray },
        { Color::background, c.bg0 },
        { Color::cursor, c.fg1 },
        { Color::selection, c.bg3 },
        { Color::whitespace, c.bg2 },
        { Color::matchingBracketBackground, c.bg3 },
        { Color::matchingBracketActive, c.fg4 },
        { Color::matchingBracketLevel1, kBracketLevels[0] },
        { Color::matchingBracketLevel2, kBracketLevels[1] },
        { Color::matchingBracketLevel3, kBracketLevels[2] },
        { Color::matchingBracketError, c.red },
        { Color::lineNumber, c.bg4 },
        { Color::currentLineNumber, c.yellow },
        { Color::currentLineHighlight, c.bg1 },
        { Color::currentLineHighlightBorder, c.bg1 },
    });
}

Theme gruvboxTheme(const char* name, const GruvboxColors& colors) {
    return { name, widgetStyleColors(widgetColors(colors)), gruvboxPalette(colors), hexColor(colors.yellow), hexColor(colors.red), hexColor(colors.yellow) };
}

}

Theme gruvboxDarkTheme() { return gruvboxTheme("Gruvbox Dark", kDark); }

Theme gruvboxLightTheme() { return gruvboxTheme("Gruvbox Light", kLight); }

}
