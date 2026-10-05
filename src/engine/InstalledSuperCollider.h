#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace supercollidaw {

std::string ugenPluginPath(const std::filesystem::path& bundledDir);
std::optional<std::string> installedSclangPath();

extern const char kSclangName[];
std::vector<std::filesystem::path> platformInstallDirs();
std::vector<std::filesystem::path> systemPluginDirs();

}
