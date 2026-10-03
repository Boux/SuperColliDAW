#pragma once

#include "Example.h"

#include <TextEditor.h>
#include <imgui.h>

#include <vector>

namespace supercollidaw {

class ExamplesPanel {
public:
    explicit ExamplesPanel(std::vector<Example> examples);

    void draw(const ImVec2& size);
    bool focused() const { return mFocused; }
    const TextEditor& viewer() const { return mViewer; }

private:
    void drawContents();
    void drawList();
    void drawListItem(size_t index);
    void drawCopyButton();
    void select(size_t index);

    std::vector<Example> mExamples;
    size_t mSelected = 0;
    TextEditor mViewer;
    bool mFocused = false;
};

}
