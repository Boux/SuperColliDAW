#include "CodeEditor.h"

namespace supercollidaw {

// The widget doesn't report its focus, so it sits in a child window of its own that can be asked.
void CodeEditor::draw(const char* id, const ImVec2& size) {
    ImGui::BeginChild(id, size, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    Render(id);
    mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    ImGui::EndChild();
}

// Same placement as the widget's own caret, valid for the frame that was just rendered.
ImVec2 CodeEditor::screenPosition(DocPos position) const {
    const VisPos visible = docPos2VisPos(normalizePos(position));
    return ImVec2(cursorScreenPos.x + textLeftOffset + visible.column * glyphSize.x, cursorScreenPos.y + visible.row * glyphSize.y);
}

std::vector<std::string> linesOf(const TextEditor& editor) {
    std::vector<std::string> lines(editor.GetLineCount());
    for (size_t line = 0; line < lines.size(); ++line)
        lines[line] = editor.GetLineText(line);
    return lines;
}

std::vector<std::string> linesBeforeCursor(const TextEditor& editor) {
    const TextEditor::DocPos cursor = editor.GetMainCursorPosition();
    std::vector<std::string> lines(cursor.line + 1);
    for (size_t line = 0; line < cursor.line; ++line)
        lines[line] = editor.GetLineText(line);
    lines[cursor.line] = editor.GetSectionText(TextEditor::DocPos(cursor.line, 0), cursor);
    return lines;
}

}
