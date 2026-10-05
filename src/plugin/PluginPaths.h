#pragma once

#include <filesystem>

namespace supercollidaw {

void setPluginBinaryPath(const char* path);
std::filesystem::path pluginResourcesDir();
std::filesystem::path userDataDir();

std::filesystem::path environmentPath(const char* name);
std::filesystem::path platformUserDataDir();

}
