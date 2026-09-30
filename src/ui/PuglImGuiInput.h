#pragma once

#include <pugl/pugl.h>

struct ImGuiIO;

namespace supercollidaw {

void forwardToImGui(ImGuiIO& io, const PuglEvent& event);
PuglCursor puglCursorFor(int imguiCursor);

}
