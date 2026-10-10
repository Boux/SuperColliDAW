#include "PuglImGuiInput.h"

#include <imgui.h>

#include <cfloat>
#include <unordered_map>

namespace supercollidaw {

namespace {

const std::unordered_map<uint32_t, ImGuiKey> kSpecialKeys = {
    { PUGL_KEY_BACKSPACE, ImGuiKey_Backspace }, { PUGL_KEY_TAB, ImGuiKey_Tab },
    { PUGL_KEY_ENTER, ImGuiKey_Enter },         { PUGL_KEY_ESCAPE, ImGuiKey_Escape },
    { PUGL_KEY_DELETE, ImGuiKey_Delete },       { PUGL_KEY_SPACE, ImGuiKey_Space },
    { PUGL_KEY_PAGE_UP, ImGuiKey_PageUp },      { PUGL_KEY_PAGE_DOWN, ImGuiKey_PageDown },
    { PUGL_KEY_END, ImGuiKey_End },             { PUGL_KEY_HOME, ImGuiKey_Home },
    { PUGL_KEY_LEFT, ImGuiKey_LeftArrow },      { PUGL_KEY_UP, ImGuiKey_UpArrow },
    { PUGL_KEY_RIGHT, ImGuiKey_RightArrow },    { PUGL_KEY_DOWN, ImGuiKey_DownArrow },
    { PUGL_KEY_INSERT, ImGuiKey_Insert },       { PUGL_KEY_MENU, ImGuiKey_Menu },
    { PUGL_KEY_SHIFT_L, ImGuiKey_LeftShift },   { PUGL_KEY_SHIFT_R, ImGuiKey_RightShift },
    { PUGL_KEY_CTRL_L, ImGuiKey_LeftCtrl },     { PUGL_KEY_CTRL_R, ImGuiKey_RightCtrl },
    { PUGL_KEY_ALT_L, ImGuiKey_LeftAlt },       { PUGL_KEY_ALT_R, ImGuiKey_RightAlt },
    { PUGL_KEY_SUPER_L, ImGuiKey_LeftSuper },   { PUGL_KEY_SUPER_R, ImGuiKey_RightSuper },
    { PUGL_KEY_PAD_ENTER, ImGuiKey_KeypadEnter }, { '\'', ImGuiKey_Apostrophe },
    { ',', ImGuiKey_Comma },                    { '-', ImGuiKey_Minus },
    { '.', ImGuiKey_Period },                   { '/', ImGuiKey_Slash },
    { ';', ImGuiKey_Semicolon },                { '=', ImGuiKey_Equal },
    { '[', ImGuiKey_LeftBracket },              { '\\', ImGuiKey_Backslash },
    { ']', ImGuiKey_RightBracket },             { '`', ImGuiKey_GraveAccent },
};

const std::unordered_map<int, PuglCursor> kCursors = {
    { ImGuiMouseCursor_Arrow, PUGL_CURSOR_ARROW },         { ImGuiMouseCursor_TextInput, PUGL_CURSOR_CARET },
    { ImGuiMouseCursor_ResizeAll, PUGL_CURSOR_ALL_SCROLL }, { ImGuiMouseCursor_ResizeNS, PUGL_CURSOR_UP_DOWN },
    { ImGuiMouseCursor_ResizeEW, PUGL_CURSOR_LEFT_RIGHT },  { ImGuiMouseCursor_ResizeNESW, PUGL_CURSOR_UP_RIGHT_DOWN_LEFT },
    { ImGuiMouseCursor_ResizeNWSE, PUGL_CURSOR_UP_LEFT_DOWN_RIGHT }, { ImGuiMouseCursor_Hand, PUGL_CURSOR_HAND },
    { ImGuiMouseCursor_NotAllowed, PUGL_CURSOR_NO },
};

ImGuiKey toImGuiKey(uint32_t key) {
    if (key >= 'a' && key <= 'z')
        return static_cast<ImGuiKey>(ImGuiKey_A + (key - 'a'));
    if (key >= '0' && key <= '9')
        return static_cast<ImGuiKey>(ImGuiKey_0 + (key - '0'));
    if (key >= PUGL_KEY_F1 && key <= PUGL_KEY_F12)
        return static_cast<ImGuiKey>(ImGuiKey_F1 + (key - PUGL_KEY_F1));
    const auto special = kSpecialKeys.find(key);
    return special == kSpecialKeys.end() ? ImGuiKey_None : special->second;
}

void forwardModifiers(ImGuiIO& io, PuglMods state) {
    io.AddKeyEvent(ImGuiMod_Ctrl, state & PUGL_MOD_CTRL);
    io.AddKeyEvent(ImGuiMod_Shift, state & PUGL_MOD_SHIFT);
    io.AddKeyEvent(ImGuiMod_Alt, state & PUGL_MOD_ALT);
    io.AddKeyEvent(ImGuiMod_Super, state & PUGL_MOD_SUPER);
}

void forwardKey(ImGuiIO& io, const PuglKeyEvent& key, bool pressed) {
    forwardModifiers(io, key.state);
    const ImGuiKey imguiKey = toImGuiKey(unshiftedKey(key));
    if (imguiKey != ImGuiKey_None)
        io.AddKeyEvent(imguiKey, pressed);
}

void forwardText(ImGuiIO& io, const PuglTextEvent& text) {
    const bool isControlCharacter = text.character < 0x20 || text.character == 0x7f;
    if (!isControlCharacter)
        io.AddInputCharactersUTF8(text.string);
}

void forwardPosition(ImGuiIO& io, double x, double y) { io.AddMousePosEvent(static_cast<float>(x), static_cast<float>(y)); }

void forwardButton(ImGuiIO& io, const PuglButtonEvent& button, bool pressed) {
    forwardModifiers(io, button.state);
    forwardPosition(io, button.x, button.y);
    if (button.button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(static_cast<int>(button.button), pressed);
}

void forwardPointerOut(ImGuiIO& io, const PuglCrossingEvent& crossing) {
    // Window managers grab the pointer on click-to-focus, which sends crossing events while the pointer stays put.
    if (crossing.mode == PUGL_CROSSING_NORMAL)
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
}

void forwardScroll(ImGuiIO& io, const PuglScrollEvent& scroll) {
    forwardPosition(io, scroll.x, scroll.y);
    io.AddMouseWheelEvent(static_cast<float>(scroll.dx), static_cast<float>(scroll.dy));
}

}

uint32_t unshiftedKey(const PuglKeyEvent& key) {
    if (key.key >= 'A' && key.key <= 'Z')
        return key.key - 'A' + 'a';
    return key.key;
}

void forwardToImGui(ImGuiIO& io, const PuglEvent& event) {
    switch (event.type) {
    case PUGL_KEY_PRESS:
        return forwardKey(io, event.key, true);
    case PUGL_KEY_RELEASE:
        return forwardKey(io, event.key, false);
    case PUGL_TEXT:
        return forwardText(io, event.text);
    case PUGL_BUTTON_PRESS:
        return forwardButton(io, event.button, true);
    case PUGL_BUTTON_RELEASE:
        return forwardButton(io, event.button, false);
    case PUGL_MOTION:
        return forwardPosition(io, event.motion.x, event.motion.y);
    case PUGL_POINTER_IN:
        return forwardPosition(io, event.crossing.x, event.crossing.y);
    case PUGL_POINTER_OUT:
        return forwardPointerOut(io, event.crossing);
    case PUGL_SCROLL:
        return forwardScroll(io, event.scroll);
    default:
        return;
    }
}

PuglCursor puglCursorFor(int imguiCursor) {
    const auto cursor = kCursors.find(imguiCursor);
    return cursor == kCursors.end() ? PUGL_CURSOR_ARROW : cursor->second;
}

}
