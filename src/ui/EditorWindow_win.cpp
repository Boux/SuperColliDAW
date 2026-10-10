#include "EditorWindow.h"

namespace supercollidaw {

// TODO: read the system key repeat delay and rate with SPI_GETKEYBOARDDELAY and SPI_GETKEYBOARDSPEED; ImGui's defaults apply meanwhile.
void EditorWindow::useSystemKeyRepeat(ImGuiIO&) {}

// TODO: check on native Windows whether activating the plugin window without clicking the editor leaves the keyboard with the DAW.
void EditorWindow::takeFocusOnActivation() {}

int EditorWindow::eventFd() const { return -1; }

}
