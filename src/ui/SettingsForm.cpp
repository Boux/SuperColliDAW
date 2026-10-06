#include "SettingsForm.h"

#include "Fonts.h"

#include <imgui.h>

namespace supercollidaw {

namespace {

bool drawFontCombo(EditorSettings& settings) {
    if (!ImGui::BeginCombo("Font", settings.font.c_str()))
        return false;
    bool changed = false;
    for (const BundledFont& font : bundledFonts()) {
        if (!ImGui::Selectable(font.name, settings.font == font.name))
            continue;
        settings.font = font.name;
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
    const bool fontChanged = drawFontCombo(settings);
    const bool sizeChanged = drawFontSize(settings, textSize);
    return fontChanged || sizeChanged;
}

}
