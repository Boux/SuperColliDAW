#include "EditorWindow.h"

namespace supercollidaw {

// TODO: read the system key repeat delay and rate with SPI_GETKEYBOARDDELAY and SPI_GETKEYBOARDSPEED; ImGui's defaults apply meanwhile.
void EditorWindow::useSystemKeyRepeat(ImGuiIO&) {}

int EditorWindow::eventFd() const { return -1; }

}
