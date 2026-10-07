#include "Theme.h"

#include "Gruvbox.h"
#include "TomorrowNight.h"

#include <algorithm>

namespace supercollidaw {

namespace {

Theme presetTheme(const char* name, void (*styleColors)(ImGuiStyle*), const TextEditor::Palette& palette, ImU32 flash, ImU32 postError, ImU32 postWarning) {
    ImGuiStyle style;
    styleColors(&style);
    return { name, std::to_array(style.Colors), palette, flash, postError, postWarning };
}

// ImGui's light preset draws white fields on white popups, which hides the fields of the settings popup.
void lightStyleColors(ImGuiStyle* style) {
    ImGui::StyleColorsLight(style);
    style->Colors[ImGuiCol_PopupBg] = style->Colors[ImGuiCol_WindowBg];
}

}

std::span<const Theme> themes() {
    static const Theme all[] = {
        presetTheme("Dark", ImGui::StyleColorsDark, TextEditor::GetDarkPalette(), hexColor(0xc07f00), hexColor(0xff7366), hexColor(0xf2cc59)),
        presetTheme("Light", lightStyleColors, TextEditor::GetLightPalette(), hexColor(0xc07f00), hexColor(0xd1242f), hexColor(0x9a6700)),
        gruvboxDarkTheme(),
        gruvboxLightTheme(),
        tomorrowNightTheme(),
    };
    return all;
}

const Theme& themeNamed(std::string_view name) {
    const std::span<const Theme> all = themes();
    const auto found = std::ranges::find(all, name, [](const Theme& theme) { return std::string_view(theme.name); });
    return found != all.end() ? *found : all.front();
}

}
