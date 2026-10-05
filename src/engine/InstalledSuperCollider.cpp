#include "InstalledSuperCollider.h"

#include "SC_Filesystem.hpp"
#include "SC_StringParser.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <numeric>
#include <vector>

namespace supercollidaw {

namespace fs = std::filesystem;

namespace {

std::vector<fs::path> extensionDirs() {
    using DirName = SC_Filesystem::DirName;
    auto& filesystem = SC_Filesystem::instance();
    return { filesystem.getDirectory(DirName::SystemExtension), filesystem.getDirectory(DirName::UserExtension) };
}

std::vector<fs::path> executableSearchDirs() {
    std::vector<fs::path> dirs;
    const char* path = std::getenv("PATH");
    for (SC_StringParser parser(path ? path : "", SC_STRPARSE_PATHDELIMITER); !parser.AtEnd();)
        dirs.emplace_back(parser.NextToken());
    const std::vector<fs::path> installs = platformInstallDirs();
    dirs.insert(dirs.end(), installs.begin(), installs.end());
    return dirs;
}

}

std::optional<std::string> installedSclangPath() {
    const std::vector<fs::path> dirs = executableSearchDirs();
    const auto found = std::find_if(dirs.begin(), dirs.end(), [](const fs::path& dir) {
        std::error_code ec;
        return fs::is_regular_file(dir / kSclangName, ec);
    });
    if (found == dirs.end())
        return std::nullopt;
    return (*found / kSclangName).string();
}

std::string ugenPluginPath(const fs::path& bundledDir) {
    std::vector<fs::path> candidates = { bundledDir };
    const std::vector<fs::path> system = systemPluginDirs();
    const std::vector<fs::path> extensions = extensionDirs();
    candidates.insert(candidates.end(), system.begin(), system.end());
    candidates.insert(candidates.end(), extensions.begin(), extensions.end());

    std::vector<fs::path> existing;
    std::for_each(candidates.begin(), candidates.end(), [&](const fs::path& dir) {
        std::error_code ec;
        const fs::path canonical = fs::canonical(dir, ec);
        if (ec || !fs::is_directory(canonical, ec))
            return;
        if (std::find(existing.begin(), existing.end(), canonical) == existing.end())
            existing.push_back(canonical);
    });

    const std::string delimiter(1, SC_STRPARSE_PATHDELIMITER);
    return std::accumulate(existing.begin(), existing.end(), std::string(), [&](std::string path, const fs::path& dir) {
        return path.empty() ? dir.string() : path + delimiter + dir.string();
    });
}

}
