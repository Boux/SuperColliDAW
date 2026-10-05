#include "InstalledSuperCollider.h"

namespace supercollidaw {

const char kSclangName[] = "sclang";

std::vector<std::filesystem::path> platformInstallDirs() { return { "/Applications/SuperCollider.app/Contents/MacOS" }; }

std::vector<std::filesystem::path> systemPluginDirs() { return {}; }

}
