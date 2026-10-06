#include "IconButton.h"

#include <imgui.h>

namespace supercollidaw {

bool iconButton(const char* icon, const char* tooltip) {
    const bool clicked = ImGui::Button(icon);
    ImGui::SetItemTooltip("%s", tooltip);
    return clicked;
}

}
