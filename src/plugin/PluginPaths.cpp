#include "PluginPaths.h"

#include <cstdlib>
#include <string>

namespace supercollidaw {

namespace fs = std::filesystem;

namespace {

fs::path gPluginBinaryPath;

fs::path environmentPath(const char* name) {
    const char* value = std::getenv(name);
    return value ? fs::path(value) : fs::path();
}

fs::path platformUserDataDir() {
#if defined(_WIN32)
    return environmentPath("APPDATA");
#elif defined(__APPLE__)
    return environmentPath("HOME") / "Library" / "Application Support";
#else
    const fs::path xdgDataHome = environmentPath("XDG_DATA_HOME");
    return xdgDataHome.empty() ? environmentPath("HOME") / ".local" / "share" : xdgDataHome;
#endif
}

}

void setPluginBinaryPath(const char* path) { gPluginBinaryPath = path; }

fs::path pluginResourcesDir() {
    std::error_code ec;
    const fs::path binary = fs::canonical(gPluginBinaryPath, ec);
    return (ec ? gPluginBinaryPath : binary).parent_path() / "SuperColliDAW";
}

fs::path userDataDir() {
    const fs::path overridden = environmentPath("SUPERCOLLIDAW_USER_DIR");
    return overridden.empty() ? platformUserDataDir() / "SuperColliDAW" : overridden;
}

}
