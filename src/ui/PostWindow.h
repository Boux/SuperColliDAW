#pragma once

#include "lang/PostLog.h"

#include <imgui.h>

#include <deque>
#include <string>

namespace supercollidaw {

class PostWindow {
public:
    explicit PostWindow(const PostLog& log): mLog(log) {}

    void setColors(ImU32 error, ImU32 warning);
    void draw(const ImVec2& size);

private:
    void pullNewLines();
    void drawLines();
    const ImU32* colorFor(const std::string& line) const;
    void drawContextMenu();

    const PostLog& mLog;
    uint64_t mNextLine = 0;
    std::deque<std::string> mLines;
    bool mHasNewLines = false;
    ImU32 mErrorColor = 0;
    ImU32 mWarningColor = 0;
};

}
