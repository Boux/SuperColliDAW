#include "PluginPaths.h"

namespace supercollidaw {

std::filesystem::path platformUserDataDir() { return environmentPath("HOME") / "Library" / "Application Support"; }

}
