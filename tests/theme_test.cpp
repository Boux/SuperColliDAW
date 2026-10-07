#include "ui/EditorSettings.h"
#include "ui/theme/Theme.h"

#include <algorithm>
#include <cstdio>
#include <ranges>
#include <set>
#include <string_view>

namespace {

using supercollidaw::Theme;
using supercollidaw::themeNamed;
using supercollidaw::themes;

int gFailures = 0;

void check(const char* name, bool passed) {
    std::printf("%s: %s\n", passed ? "PASS" : "FAIL", name);
    gFailures += passed ? 0 : 1;
}

// A color the mapping forgot is all zeros, while one made transparent on purpose keeps its RGB. ImGui's own presets break this rule, so only hand-written themes are checked.
bool setsEveryStyleColor(const Theme& theme) {
    return std::ranges::none_of(theme.styleColors, [](const ImVec4& color) { return color.x == 0.f && color.y == 0.f && color.z == 0.f && color.w == 0.f; });
}

bool setsEveryPaletteColor(const Theme& theme) { return std::ranges::none_of(theme.palette, [](ImU32 color) { return color == 0; }); }

}

int main() {
    const auto nameList = themes() | std::views::transform(&Theme::name);
    const std::set<std::string_view> names(nameList.begin(), nameList.end());
    check("theme names are unique", names.size() == themes().size());
    check("the default theme is the one an unknown name falls back to", std::string_view(themes().front().name) == supercollidaw::kDefaultTheme);
    check("an unknown name gives the first theme", &themeNamed("Solarized") == &themes().front());
    check("a known name gives that theme", std::string_view(themeNamed("Gruvbox Light").name) == "Gruvbox Light");
    check("Gruvbox Dark sets every ImGui color", setsEveryStyleColor(themeNamed("Gruvbox Dark")));
    check("Gruvbox Light sets every ImGui color", setsEveryStyleColor(themeNamed("Gruvbox Light")));
    check("Tomorrow Night sets every ImGui color", setsEveryStyleColor(themeNamed("Tomorrow Night")));
    check("every theme sets every editor color", std::ranges::all_of(themes(), setsEveryPaletteColor));
    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
