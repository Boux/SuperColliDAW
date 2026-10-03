#pragma once

#include "EditorActions.h"
#include "ExamplesPanel.h"
#include "PostWindow.h"

#include <TextEditor.h>

#include <string>
#include <vector>

namespace supercollidaw {

class EditorView {
public:
    EditorView(EditorActions actions, const PostLog& postLog, std::vector<Example> examples);

    void setCode(const std::string& code);
    void setStatus(EditorStatus status) { mStatus = std::move(status); }
    void draw();

private:
    void drawToolbar();
    void drawFileButtons();
    void drawExamplesButton();
    void drawStatus();
    void drawCodeAndPost();
    void drawExamples(float width);
    float examplesWidth(float width) const;
    void handleShortcuts();
    const TextEditor& focusedEditor() const;

    EditorActions mActions;
    EditorStatus mStatus;
    TextEditor mEditor;
    PostWindow mPostWindow;
    ExamplesPanel mExamples;
    float mPostHeight;
    float mExamplesShare;
    bool mShowExamples = false;
};

}
