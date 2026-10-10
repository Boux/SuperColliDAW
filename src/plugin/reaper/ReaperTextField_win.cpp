#include "ReaperTextField.h"

#include <reaper_plugin.h>

#include <algorithm>
#include <vector>

namespace supercollidaw {

namespace {

constexpr char kReaperExtension[] = "cockos.reaper_extension";

// hwnd_info query and answers from reaper_plugin.h.
constexpr INT_PTR kIsTextField = 0;
constexpr int kTextField = 1;
constexpr int kUnknownWindow = 0;

// REAPER passes hwnd_info callbacks no context, so every open editor's window shares one list.
std::vector<HWND>& textFields() {
    static std::vector<HWND> windows;
    return windows;
}

int hwndInfo(HWND hwnd, INT_PTR query) {
    const bool isEditor = query == kIsTextField && std::ranges::find(textFields(), hwnd) != textFields().end();
    return isEditor ? kTextField : kUnknownWindow;
}

}

ReaperTextField::ReaperTextField(const clap_host* host, PuglNativeView view): mView(view) {
    const auto* reaper = static_cast<const reaper_plugin_info_t*>(host->get_extension(host, kReaperExtension));
    if (!reaper)
        return;
    mRegister = reaper->Register;
    if (textFields().empty())
        mRegister("hwnd_info", reinterpret_cast<void*>(hwndInfo));
    textFields().push_back(reinterpret_cast<HWND>(view));
}

ReaperTextField::~ReaperTextField() {
    if (!mRegister)
        return;
    std::erase(textFields(), reinterpret_cast<HWND>(mView));
    if (textFields().empty())
        mRegister("-hwnd_info", reinterpret_cast<void*>(hwndInfo));
}

}
