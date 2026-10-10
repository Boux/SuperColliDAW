#pragma once

#include <pugl/pugl.h>

struct ImGuiIO;

namespace supercollidaw {

// pugl's X11 backend leaves Caps Lock applied to PuglKeyEvent::key, so letters can arrive uppercase.
uint32_t unshiftedKey(const PuglKeyEvent& key);
void forwardToImGui(ImGuiIO& io, const PuglEvent& event);
PuglCursor puglCursorFor(int imguiCursor);

}
