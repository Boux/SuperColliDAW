#include "PostWindow.h"

#include <numeric>
#include <vector>

namespace supercollidaw {

namespace {

constexpr ImVec4 kErrorColor(1.f, 0.45f, 0.4f, 1.f);
constexpr ImVec4 kWarningColor(0.95f, 0.8f, 0.35f, 1.f);

bool contains(const std::string& line, const char* text) { return line.find(text) != std::string::npos; }

const ImVec4* colorFor(const std::string& line) {
    if (contains(line, "ERROR") || contains(line, "FAILURE IN SERVER"))
        return &kErrorColor;
    if (contains(line, "WARNING"))
        return &kWarningColor;
    return nullptr;
}

void drawLine(const std::string& line) {
    const ImVec4* color = colorFor(line);
    if (color)
        ImGui::PushStyleColor(ImGuiCol_Text, *color);
    ImGui::TextUnformatted(line.data(), line.data() + line.size());
    if (color)
        ImGui::PopStyleColor();
}

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
            drawLine(mLines[line]);
    }
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
