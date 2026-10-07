#include "TomorrowNight.h"

#include "ColorScheme.h"

namespace supercollidaw {

namespace {

using Color = TextEditor::Color;

// From chriskempson/tomorrow-theme vim/colors/Tomorrow-Night.vim, whose colors are also Alacritty's default before 0.13.
constexpr uint32_t kForeground = 0xc5c8c6;
constexpr uint32_t kBackground = 0x1d1f21;
constexpr uint32_t kSelection = 0x373b41;
constexpr uint32_t kLine = 0x282a2e;
constexpr uint32_t kComment = 0x969896;
constexpr uint32_t kRed = 0xcc6666;
constexpr uint32_t kOrange = 0xde935f;
constexpr uint32_t kYellow = 0xf0c674;
constexpr uint32_t kGreen = 0xb5bd68;
constexpr uint32_t kAqua = 0x8abeb7;
constexpr uint32_t kBlue = 0x81a2be;
constexpr uint32_t kPurple = 0xb294bb;
constexpr uint32_t kWindow = 0x4d5057;
// Alacritty's dim and bright black, for the shades the vim scheme has no name for.
constexpr uint32_t kDimBlack = 0x131415;
constexpr uint32_t kBrightBlack = 0x666666;

constexpr WidgetColors kWidgetColors = { .windowBg = kDimBlack, .bg1 = kLine, .bg2 = kSelection, .bg3 = kWindow, .bg4 = kBrightBlack, .text = kForeground, .textDisabled = kComment, .accent = kBlue, .accentActive = kAqua, .highlight = kYellow, .highlightActive = kOrange };

// Token colors follow scnvim's syntax groups as Tomorrow-Night.vim colors them: class names are Identifier (red), symbols and numbers Constant (orange), operators Special (foreground). The scheme has no rainbow brackets, so those take its unused accents.
TextEditor::Palette tomorrowNightPalette() {
    return editorPalette({
        { Color::text, kForeground },
        { Color::keyword, kOrange },
        { Color::declaration, kRed },
        { Color::number, kOrange },
        { Color::string, kGreen },
        { Color::punctuation, kForeground },
        { Color::preprocessor, kPurple },
        { Color::identifier, kForeground },
        { Color::knownIdentifier, kOrange },
        { Color::comment, kComment },
        { Color::background, kBackground },
        { Color::cursor, kForeground },
        { Color::selection, kSelection },
        { Color::whitespace, kSelection },
        { Color::matchingBracketBackground, kSelection },
        { Color::matchingBracketActive, kComment },
        { Color::matchingBracketLevel1, kBlue },
        { Color::matchingBracketLevel2, kPurple },
        { Color::matchingBracketLevel3, kAqua },
        { Color::matchingBracketError, kRed },
        { Color::lineNumber, kSelection },
        { Color::currentLineNumber, kYellow },
        { Color::currentLineHighlight, kLine },
        { Color::currentLineHighlightBorder, kLine },
    });
}

}

Theme tomorrowNightTheme() { return { "Tomorrow Night", widgetStyleColors(kWidgetColors), tomorrowNightPalette(), hexColor(kYellow), hexColor(kRed), hexColor(kYellow) }; }

}
