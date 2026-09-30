#include "DefaultCode.h"

#include "plugin/PluginPaths.h"

#include <fstream>

extern const unsigned char supercollidaw_default_scd[];
extern const unsigned long supercollidaw_default_scd_size;

namespace supercollidaw {

namespace fs = std::filesystem;

fs::path ensureDefaultCodeFile() {
    const fs::path path = userDataDir() / "default.scd";
    std::error_code ec;
    if (fs::exists(path, ec))
        return path;
    fs::create_directories(path.parent_path(), ec);
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(supercollidaw_default_scd), supercollidaw_default_scd_size);
    return path;
}

}
