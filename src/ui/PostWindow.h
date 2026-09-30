#pragma once

#include "lang/PostLog.h"

#include <imgui.h>

#include <deque>
#include <string>

namespace supercollidaw {

class PostWindow {
public:
    explicit PostWindow(const PostLog& log): mLog(log) {}

    void draw(const ImVec2& size);

private:
    void pullNewLines();
    void drawLines();
    void drawContextMenu();

    const PostLog& mLog;
    uint64_t mNextLine = 0;
    std::deque<std::string> mLines;
    bool mHasNewLines = false;
};

}
