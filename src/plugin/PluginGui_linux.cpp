#include "PluginGui.h"

namespace supercollidaw {

const char* const PluginGui::kWindowApi = CLAP_WINDOW_API_X11;

PuglNativeView PluginGui::nativeParent(const clap_window& window) { return static_cast<PuglNativeView>(window.x11); }

}
