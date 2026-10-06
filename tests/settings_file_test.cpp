#include "plugin/settings/SettingsFile.h"

#include <cstdio>

namespace {

using supercollidaw::EditorSettings;
using supercollidaw::formatSettings;
using supercollidaw::parseSettings;

int gFailures = 0;

void check(const char* name, bool passed) {
    std::printf("%s: %s\n", passed ? "PASS" : "FAIL", name);
    gFailures += passed ? 0 : 1;
}

}

int main() {
    const EditorSettings chosen{ .font = "Cozette", .fontSizePercent = 150 };
    check("formatted settings read back the same", parseSettings(formatSettings(chosen)) == chosen);
    check("an empty file gives the defaults", parseSettings("") == EditorSettings{});
    check("spaces and CRLF line ends are ignored", parseSettings("  font =  JetBrains Mono \r\nfont_size_percent= 80\r\n") == EditorSettings{ .font = "JetBrains Mono", .fontSizePercent = 80 });
    check("unknown keys and lines without = are ignored", parseSettings("theme = gruvbox\njunk\nfont_size_percent = 120\n") == EditorSettings{ .fontSizePercent = 120 });
    check("a size that is not a number keeps the default", parseSettings("font_size_percent = big\n").fontSizePercent == 100);
    check("a size out of range is clamped", parseSettings("font_size_percent = 900\n").fontSizePercent == EditorSettings::kMaxFontSizePercent);
    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
