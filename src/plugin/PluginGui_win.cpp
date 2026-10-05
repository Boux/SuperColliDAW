#include "PluginGui.h"

namespace supercollidaw {

const char* const PluginGui::kWindowApi = CLAP_WINDOW_API_WIN32;

PuglNativeView PluginGui::nativeParent(const clap_window& window) { return reinterpret_cast<PuglNativeView>(window.win32); }

}
