#include "EditorWindow.h"

#include <imgui.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>

#include <utility>

namespace supercollidaw {

namespace {

// The host's windows can disappear at any time, and an X error on them would reach the host's own error handler, which may exit.
class ScopedErrorTrap {
public:
    explicit ScopedErrorTrap(Display* display): mDisplay(display), mPrevious(XSetErrorHandler(ignoreError)) {}
    ~ScopedErrorTrap() {
        XSync(mDisplay, False);
        XSetErrorHandler(mPrevious);
    }
    ScopedErrorTrap(const ScopedErrorTrap&) = delete;
    ScopedErrorTrap& operator=(const ScopedErrorTrap&) = delete;

private:
    static int ignoreError(Display*, XErrorEvent*) { return 0; }

    Display* mDisplay;
    XErrorHandler mPrevious;
};

Window activeWindow(Display* display) {
    Atom type = None;
    int format = 0;
    unsigned long count = 0;
    unsigned long remaining = 0;
    unsigned char* data = nullptr;
    const Atom property = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    if (XGetWindowProperty(display, DefaultRootWindow(display), property, 0, 1, False, XA_WINDOW, &type, &format, &count, &remaining, &data) != Success)
        return None;
    const Window window = count == 1 && format == 32 ? *reinterpret_cast<Window*>(data) : None;
    XFree(data);
    return window;
}

Window parentOf(Display* display, Window window) {
    Window root = None;
    Window parent = None;
    Window* children = nullptr;
    unsigned int count = 0;
    if (!XQueryTree(display, window, &root, &parent, &children, &count))
        return None;
    XFree(children);
    return parent == root ? None : parent;
}

bool contains(Display* display, Window ancestor, Window window) {
    for (Window current = window; current != None; current = parentOf(display, current)) {
        if (current == ancestor)
            return true;
    }
    return false;
}

bool pointerInside(Display* display, Window window) {
    Window root = None;
    Window child = None;
    int rootX = 0;
    int rootY = 0;
    int x = 0;
    int y = 0;
    unsigned int buttons = 0;
    XWindowAttributes attributes;
    if (!XQueryPointer(display, window, &root, &child, &rootX, &rootY, &x, &y, &buttons) || !XGetWindowAttributes(display, window, &attributes))
        return false;
    return x >= 0 && y >= 0 && x < attributes.width && y < attributes.height;
}

}

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

// The pointer over the host's part of the window means the host was meant to get the keyboard, e.g. a plugin docked in the DAW's main window.
void EditorWindow::takeFocusOnActivation() {
    Display* display = static_cast<Display*>(puglGetNativeWorld(mWorld));
    const Window active = activeWindow(display);
    if (active == std::exchange(mActiveWindow, active) || active == None)
        return;
    const Window view = puglGetNativeView(mView);
    ScopedErrorTrap trap(display);
    if (!contains(display, active, view) || (pointerInside(display, active) && !pointerInside(display, view)))
        return;
    puglGrabFocus(mView);
}

uint32_t EditorWindow::toPixels(uint32_t size) const { return size; }

int EditorWindow::eventFd() const { return ConnectionNumber(static_cast<Display*>(puglGetNativeWorld(mWorld))); }

}
