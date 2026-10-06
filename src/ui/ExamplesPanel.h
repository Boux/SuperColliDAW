#pragma once

#include "Example.h"
#include "IconButtons.h"

#include <TextEditor.h>
#include <imgui.h>

#include <vector>

namespace supercollidaw {

class ExamplesPanel {
public:
    ExamplesPanel(std::vector<Example> examples, const IconButtons& icons);

    void draw(const ImVec2& size);
    bool focused() const { return mFocused; }
    TextEditor& viewer() { return mViewer; }

private:
    void drawContents();
    void drawCombo();
    void drawComboItem(size_t index);
    void drawCopyButton();
    void select(size_t index);

    std::vector<Example> mExamples;
    const IconButtons& mIcons;
    size_t mSelected = 0;
    TextEditor mViewer;
    bool mFocused = false;
};

}
