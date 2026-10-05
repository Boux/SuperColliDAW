#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace supercollidaw {

std::string ugenPluginPath(const std::filesystem::path& bundledDir);
std::optional<std::string> installedSclangPath();

}
