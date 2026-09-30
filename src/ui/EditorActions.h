#pragma once

#include <functional>
#include <string>

namespace supercollidaw {

struct EditorActions {
    std::function<void(const std::string& code)> runAll;
    std::function<void(const std::string& code)> evaluate;
    std::function<void()> stop;
    std::function<void(const std::string& code)> codeChanged;
};

}
