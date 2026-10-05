#include "EditorWindow.h"

#include <imgui.h>
#include <X11/XKBlib.h>

namespace supercollidaw {

void EditorWindow::useSystemKeyRepeat(ImGuiIO& io) {
    Display* display = static_cast<Display*>(puglGetNativeWorld(mWorld));
    // X11 repeats held keys as release+press pairs, which ImGui trickles over two frames each, so they lag behind.
    XkbSetDetectableAutoRepeat(display, True, nullptr);
    unsigned int delayMs = 0;
    unsigned int intervalMs = 0;
    if (!XkbGetAutoRepeatRate(display, XkbUseCoreKbd, &delayMs, &intervalMs))
        return;
    io.KeyRepeatDelay = delayMs / 1000.f;
    io.KeyRepeatRate = intervalMs / 1000.f;
}

int EditorWindow::eventFd() const { return ConnectionNumber(static_cast<Display*>(puglGetNativeWorld(mWorld))); }

}
