#include "SettingsForm.h"

#include "Fonts.h"
#include "theme/Theme.h"

#include <imgui.h>

#include <ranges>
#include <string>

namespace supercollidaw {

namespace {

std::string choiceName(const std::string& value) { return value; }
std::string choiceName(int value) { return std::to_string(value); }

// Lists the choices by name and stores the picked one; returns true on a pick.
template <typename Value, typename Choices>
bool drawCombo(const char* label, Value& selected, const Choices& choices) {
    if (!ImGui::BeginCombo(label, choiceName(selected).c_str()))
        return false;
    bool changed = false;
    for (const Value choice : choices) {
        if (!ImGui::Selectable(choiceName(choice).c_str(), selected == choice))
            continue;
        selected = choice;
        changed = true;
    }
    ImGui::EndCombo();
    return changed;
}

// The size applies while dragging, and only the release is a change worth saving.
bool drawFontSize(EditorSettings& settings, float textSize) {
    ImGui::SliderInt("Size", &settings.fontSizePercent, EditorSettings::kMinFontSizePercent, EditorSettings::kMaxFontSizePercent, "%d%%",
        ImGuiSliderFlags_AlwaysClamp);
    const bool released = ImGui::IsItemDeactivatedAfterEdit();
    ImGui::SameLine();
    ImGui::TextDisabled("%.0f px", textSize);
    return released;
}

}

bool drawSettingsForm(EditorSettings& settings, float textSize) {
    const bool themeChanged = drawCombo("Theme", settings.theme, themes() | std::views::transform(&Theme::name));
    const bool fontChanged = drawCombo("Font", settings.font, bundledFonts() | std::views::transform(&BundledFont::name));
    const bool sizeChanged = drawFontSize(settings, textSize);
    ImGui::Separator();
    const bool tabSizeChanged = drawCombo("Tab size", settings.tabSize, EditorSettings::kTabSizes);
    const bool indentChanged = ImGui::Checkbox("Indent with spaces", &settings.indentWithSpaces);
    const bool bracketsChanged = ImGui::Checkbox("Close brackets and quotes", &settings.closeBrackets);
    return themeChanged || fontChanged || sizeChanged || tabSizeChanged || indentChanged || bracketsChanged;
}

}
