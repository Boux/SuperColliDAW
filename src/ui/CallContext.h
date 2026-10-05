#pragma once

#include <optional>
#include <string>
#include <vector>

namespace supercollidaw {

struct CallContext {
    std::string callee;
    size_t argument = 0;
    std::string keyword;
};

std::optional<CallContext> enclosingCall(const std::vector<std::string>& linesBeforeCursor);

}
