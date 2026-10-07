#include "PostWindow.h"

#include <numeric>
#include <vector>

namespace supercollidaw {

namespace {

bool contains(const std::string& line, const char* text) { return line.find(text) != std::string::npos; }

void drawLine(const std::string& line, const ImU32* color) {
    if (color)
        ImGui::PushStyleColor(ImGuiCol_Text, *color);
    ImGui::TextUnformatted(line.data(), line.data() + line.size());
    if (color)
        ImGui::PopStyleColor();
}

}

void PostWindow::setColors(ImU32 error, ImU32 warning) {
    mErrorColor = error;
    mWarningColor = warning;
}

void PostWindow::draw(const ImVec2& size) {
    pullNewLines();
    ImGui::BeginChild("post", size, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    const bool followOutput = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
    drawLines();
    if (mHasNewLines && followOutput)
        ImGui::SetScrollHereY(1.f);
    mHasNewLines = false;
    drawContextMenu();
    ImGui::EndChild();
}

void PostWindow::pullNewLines() {
    std::vector<std::string> lines;
    mLog.collect(mNextLine, lines);
    mHasNewLines = mHasNewLines || !lines.empty();
    mLines.insert(mLines.end(), lines.begin(), lines.end());
    while (mLines.size() > PostLog::kMaxLines)
        mLines.pop_front();
}

void PostWindow::drawLines() {
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(mLines.size()));
    while (clipper.Step()) {
        for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; ++line)
            drawLine(mLines[line], colorFor(mLines[line]));
    }
}

const ImU32* PostWindow::colorFor(const std::string& line) const {
    if (contains(line, "ERROR") || contains(line, "FAILURE IN SERVER"))
        return &mErrorColor;
    if (contains(line, "WARNING"))
        return &mWarningColor;
    return nullptr;
}

void PostWindow::drawContextMenu() {
    if (!ImGui::BeginPopupContextWindow())
        return;
    if (ImGui::MenuItem("Copy all")) {
        const std::string text = std::accumulate(mLines.begin(), mLines.end(), std::string(), [](std::string all, const std::string& line) { return all + line + '\n'; });
        ImGui::SetClipboardText(text.c_str());
    }
    if (ImGui::MenuItem("Clear"))
        mLines.clear();
    ImGui::EndPopup();
}

}
