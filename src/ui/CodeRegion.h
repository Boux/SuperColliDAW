#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace supercollidaw {

struct LineRange {
    size_t first;
    size_t last;
};

// Mirrors the SC IDE's Ctrl+Enter region: "(" and ")" at the start of a line, else the cursor's line.
LineRange regionAround(const std::vector<std::string>& lines, size_t cursorLine);

}
