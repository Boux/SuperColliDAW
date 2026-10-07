#pragma once

#include "CodeRegion.h"

#include <TextEditor.h>

#include <chrono>

namespace supercollidaw {

class CodeFlash {
public:
    void start(TextEditor& editor, LineRange lines);
    void setColor(ImU32 color) { mColor = color; }
    void update();

private:
    TextEditor* mEditor = nullptr;
    LineRange mLines{};
    std::chrono::steady_clock::time_point mStart;
    ImU32 mColor = 0;
};

}
