#include "ColorScheme.h"

namespace supercollidaw {

std::array<ImVec4, ImGuiCol_COUNT> widgetStyleColors(const WidgetColors& c) {
    std::array<ImVec4, ImGuiCol_COUNT> colors{};
    const auto set = [&colors](ImGuiCol color, uint32_t rgb, float alpha = 1.f) { colors[color] = ImGui::ColorConvertU32ToFloat4(hexColor(rgb, alpha)); };
    set(ImGuiCol_Text, c.text);
    set(ImGuiCol_TextDisabled, c.textDisabled);
    set(ImGuiCol_WindowBg, c.windowBg);
    set(ImGuiCol_ChildBg, c.windowBg, 0.f);
    set(ImGuiCol_PopupBg, c.bg1);
    set(ImGuiCol_Border, c.bg2);
    set(ImGuiCol_BorderShadow, c.windowBg, 0.f);
    set(ImGuiCol_FrameBg, c.bg2);
    set(ImGuiCol_FrameBgHovered, c.bg3);
    set(ImGuiCol_FrameBgActive, c.bg4);
    set(ImGuiCol_TitleBg, c.windowBg);
    set(ImGuiCol_TitleBgActive, c.bg1);
    set(ImGuiCol_TitleBgCollapsed, c.windowBg);
    set(ImGuiCol_MenuBarBg, c.bg1);
    set(ImGuiCol_ScrollbarBg, c.windowBg, 0.f);
    set(ImGuiCol_ScrollbarGrab, c.bg2);
    set(ImGuiCol_ScrollbarGrabHovered, c.bg3);
    set(ImGuiCol_ScrollbarGrabActive, c.bg4);
    set(ImGuiCol_CheckMark, c.accent);
    set(ImGuiCol_CheckboxSelectedBg, c.bg3);
    set(ImGuiCol_SliderGrab, c.accent);
    set(ImGuiCol_SliderGrabActive, c.accentActive);
    set(ImGuiCol_Button, c.bg2);
    set(ImGuiCol_ButtonHovered, c.bg3);
    set(ImGuiCol_ButtonActive, c.bg4);
    set(ImGuiCol_Header, c.bg2);
    set(ImGuiCol_HeaderHovered, c.bg3);
    set(ImGuiCol_HeaderActive, c.bg4);
    set(ImGuiCol_Separator, c.bg2);
    set(ImGuiCol_SeparatorHovered, c.bg4);
    set(ImGuiCol_SeparatorActive, c.accent);
    set(ImGuiCol_ResizeGrip, c.bg2);
    set(ImGuiCol_ResizeGripHovered, c.bg3);
    set(ImGuiCol_ResizeGripActive, c.bg4);
    set(ImGuiCol_InputTextCursor, c.text);
    set(ImGuiCol_TabHovered, c.bg3);
    set(ImGuiCol_Tab, c.bg1);
    set(ImGuiCol_TabSelected, c.bg2);
    set(ImGuiCol_TabSelectedOverline, c.accent);
    set(ImGuiCol_TabDimmed, c.windowBg);
    set(ImGuiCol_TabDimmedSelected, c.bg1);
    set(ImGuiCol_TabDimmedSelectedOverline, c.bg4, 0.f);
    set(ImGuiCol_PlotLines, c.textDisabled);
    set(ImGuiCol_PlotLinesHovered, c.highlightActive);
    set(ImGuiCol_PlotHistogram, c.highlight);
    set(ImGuiCol_PlotHistogramHovered, c.highlightActive);
    set(ImGuiCol_TableHeaderBg, c.bg1);
    set(ImGuiCol_TableBorderStrong, c.bg3);
    set(ImGuiCol_TableBorderLight, c.bg2);
    set(ImGuiCol_TableRowBg, c.windowBg, 0.f);
    set(ImGuiCol_TableRowBgAlt, c.text, 0.06f);
    set(ImGuiCol_TextLink, c.accent);
    set(ImGuiCol_TextSelectedBg, c.accent, 0.35f);
    set(ImGuiCol_TreeLines, c.bg2);
    set(ImGuiCol_DragDropTarget, c.highlight);
    set(ImGuiCol_DragDropTargetBg, c.windowBg, 0.f);
    set(ImGuiCol_UnsavedMarker, c.text);
    set(ImGuiCol_NavCursor, c.accent);
    set(ImGuiCol_NavWindowingHighlight, c.text, 0.7f);
    set(ImGuiCol_NavWindowingDimBg, c.bg4, 0.2f);
    set(ImGuiCol_ModalWindowDimBg, c.bg4, 0.35f);
    return colors;
}

TextEditor::Palette editorPalette(std::initializer_list<std::pair<TextEditor::Color, uint32_t>> colors) {
    TextEditor::Palette palette{};
    for (const auto& [color, rgb] : colors)
        palette[static_cast<size_t>(color)] = hexColor(rgb);
    return palette;
}

}
