#pragma once

#include <TextEditor.h>

#include <string>
#include <vector>

namespace supercollidaw {

class CodeEditor : public TextEditor {
public:
    void draw(const char* id, const ImVec2& size);
    bool focused() const { return mFocused; }
    ImVec2 screenPosition(DocPos position) const;

private:
    bool mFocused = false;
};

std::vector<std::string> linesOf(const TextEditor& editor);
std::vector<std::string> linesBeforeCursor(const TextEditor& editor);

}
