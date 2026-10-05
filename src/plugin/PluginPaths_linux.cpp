#include "PluginPaths.h"

namespace supercollidaw {

std::filesystem::path platformUserDataDir() {
    const std::filesystem::path xdgDataHome = environmentPath("XDG_DATA_HOME");
    return xdgDataHome.empty() ? environmentPath("HOME") / ".local" / "share" : xdgDataHome;
}

}
