#include "PuglClipboard.h"

#include "PuglImGuiInput.h"

#include <imgui.h>

#include <climits>
#include <cstring>

namespace supercollidaw {

namespace {

constexpr char kTextType[] = "text/plain";

PuglClipboard& clipboardOf(ImGuiContext*) { return *static_cast<PuglClipboard*>(ImGui::GetPlatformIO().Platform_ClipboardUserData); }

bool isPasteShortcut(const PuglKeyEvent& key) {
    const bool ctrlV = (key.state & PUGL_MOD_CTRL) && unshiftedKey(key) == 'v';
    const bool shiftInsert = (key.state & PUGL_MOD_SHIFT) && key.key == PUGL_KEY_INSERT;
    return ctrlV || shiftInsert;
}

bool isText(const char* type) { return type && !std::strncmp(type, "text/", 5); }

void replayPasteShortcut(ImGuiIO& io) {
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    io.AddKeyEvent(ImGuiKey_V, true);
    io.AddKeyEvent(ImGuiKey_V, false);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
}

}

void PuglClipboard::install() {
    ImGuiPlatformIO& platform = ImGui::GetPlatformIO();
    platform.Platform_ClipboardUserData = this;
    platform.Platform_GetClipboardTextFn = [](ImGuiContext* context) { return clipboardOf(context).mText.c_str(); };
    platform.Platform_SetClipboardTextFn = [](ImGuiContext* context, const char* text) {
        PuglClipboard& clipboard = clipboardOf(context);
        clipboard.mText = text;
        puglSetClipboard(clipboard.mView, PUGL_CLIPBOARD_GENERAL, kTextType, text, std::strlen(text));
    };
}

bool PuglClipboard::requestPasteFor(const PuglEvent& event) {
    if (event.type != PUGL_KEY_PRESS || !isPasteShortcut(event.key))
        return false;
    puglPaste(mView);
    return true;
}

PuglStatus PuglClipboard::acceptOffer(const PuglDataOfferEvent& offer) {
    const uint32_t numTypes = puglGetNumClipboardTypes(mView, offer.clipboard);
    for (uint32_t type = 0; type < numTypes; ++type) {
        if (isText(puglGetClipboardType(mView, offer.clipboard, type)))
            return puglAcceptOffer(mView, &offer, type, PUGL_DATA_ACTION_COPY, 0, 0, UINT_MAX, UINT_MAX);
    }
    return PUGL_UNSUPPORTED;
}

PuglStatus PuglClipboard::receive(const PuglDataEvent& data) {
    size_t size = 0;
    const char* text = static_cast<const char*>(puglGetClipboard(mView, data.clipboard, data.typeIndex, &size));
    if (!text)
        return PUGL_FAILURE;
    mText.assign(text, strnlen(text, size));
    replayPasteShortcut(ImGui::GetIO());
    return PUGL_SUCCESS;
}

}
