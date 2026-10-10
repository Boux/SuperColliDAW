#include "EditorWindow.h"

#include <cmath>

namespace supercollidaw {

// TODO: read the system key repeat delay and rate from NSEvent; ImGui's defaults apply meanwhile.
void EditorWindow::useSystemKeyRepeat(ImGuiIO&) {}

// TODO: check on macOS whether activating the plugin window without clicking the editor leaves the keyboard with the DAW.
void EditorWindow::takeFocusOnActivation() {}

// The host sizes a Cocoa view in points, and pugl in pixels.
uint32_t EditorWindow::toPixels(uint32_t size) const { return static_cast<uint32_t>(std::lround(size * puglGetScaleFactor(mView))); }

int EditorWindow::eventFd() const { return -1; }

}
