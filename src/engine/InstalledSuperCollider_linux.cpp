#include "InstalledSuperCollider.h"

namespace supercollidaw {

const char kSclangName[] = "sclang";

std::vector<std::filesystem::path> platformInstallDirs() { return {}; }

std::vector<std::filesystem::path> systemPluginDirs() {
    return { "/usr/lib/SuperCollider/plugins", "/usr/lib64/SuperCollider/plugins", "/usr/local/lib/SuperCollider/plugins" };
}

}
