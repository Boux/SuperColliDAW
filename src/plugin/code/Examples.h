#pragma once

#include "ui/Example.h"

#include <filesystem>
#include <vector>

namespace supercollidaw {

std::vector<Example> loadExamples(const std::filesystem::path& dir);

}
