#pragma once

#include "Theme.h"

#include <initializer_list>
#include <utility>

namespace supercollidaw {

struct WidgetColors {
    uint32_t windowBg, bg1, bg2, bg3, bg4;
    uint32_t text, textDisabled;
    uint32_t accent, accentActive, highlight, highlightActive;
};

std::array<ImVec4, ImGuiCol_COUNT> widgetStyleColors(const WidgetColors& colors);
TextEditor::Palette editorPalette(std::initializer_list<std::pair<TextEditor::Color, uint32_t>> colors);

}
