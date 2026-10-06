#include "IconButtons.h"

#include <imgui.h>

#include <cmath>

namespace supercollidaw {

namespace {

ImVec2 currentFrameSize() {
    const float size = ImGui::GetFontSize();
    const ImVec2 padding = ImGui::GetStyle().FramePadding;
    return ImVec2(size + padding.x * 2.f, size + padding.y * 2.f);
}

// Centers the icon's em square with the ascent ImGui actually baked: Lucide's ascent equals its em, and ImGui's float math rounds it up a pixel.
void drawIcon(const char* icon, const ImVec2& min, const ImVec2& max) {
    const float size = ImGui::GetFontSize();
    const float left = (min.x + max.x - size) * 0.5f;
    const float top = (min.y + max.y + size) * 0.5f - ImGui::GetFontBaked()->Ascent;
    ImGui::GetWindowDrawList()->AddText(ImVec2(std::round(left), std::round(top)), ImGui::GetColorU32(ImGuiCol_Text), icon);
}

}

bool IconButtons::button(const char* icon, const char* tooltip) const {
    ImGui::PushFont(mFont, iconSize());
    ImGui::PushID(icon);
    const bool clicked = ImGui::Button("##icon", currentFrameSize());
    ImGui::PopID();
    drawIcon(icon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImGui::PopFont();
    ImGui::SetItemTooltip("%s", tooltip);
    return clicked;
}

ImVec2 IconButtons::frameSize() const {
    ImGui::PushFont(mFont, iconSize());
    const ImVec2 size = currentFrameSize();
    ImGui::PopFont();
    return size;
}

}
