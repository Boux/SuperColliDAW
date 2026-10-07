#pragma once

#include <TextEditor.h>
#include <imgui.h>

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace supercollidaw {

struct Theme {
    const char* name;
    std::array<ImVec4, ImGuiCol_COUNT> styleColors;
    TextEditor::Palette palette;
    ImU32 flash;
    ImU32 postError;
    ImU32 postWarning;
};

constexpr ImU32 hexColor(uint32_t rgb, float alpha = 1.f) { return IM_COL32((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff, static_cast<int>(alpha * 255.f)); }

std::span<const Theme> themes();
const Theme& themeNamed(std::string_view name);

}
