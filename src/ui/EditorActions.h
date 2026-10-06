#pragma once

#include "EditorSettings.h"

#include <functional>
#include <string>

namespace supercollidaw {

struct EditorStatus {
    std::string file;
    bool dirty = false;
};

struct EditorActions {
    std::function<void(const std::string& code)> runAll;
    std::function<void(const std::string& code)> evaluate;
    std::function<void()> stop;
    std::function<void()> rebootInterpreter;
    std::function<void(const std::string& code)> codeChanged;
    std::function<void(const std::string& line)> complete;
    std::function<void(const std::string& callee)> lookUpSignatures;
    std::function<void()> open;
    std::function<void(const std::string& code)> save;
    std::function<void(const std::string& code)> saveAs;
    std::function<void(const EditorSettings& settings)> settingsChanged;
};

}
