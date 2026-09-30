#pragma once

#include "EditorActions.h"
#include "PostWindow.h"

#include <TextEditor.h>

#include <string>

namespace supercollidaw {

class EditorView {
public:
    EditorView(EditorActions actions, const PostLog& postLog);

    void setCode(const std::string& code);
    void setStatus(EditorStatus status) { mStatus = std::move(status); }
    void draw();

private:
    void drawToolbar();
    void drawFileButtons();
    void drawStatus();
    void drawSplitter();
    void handleShortcuts();
    std::string selectionOrRegion() const;
    std::string selectionOrLine() const;

    EditorActions mActions;
    EditorStatus mStatus;
    TextEditor mEditor;
    PostWindow mPostWindow;
    float mPostHeight;
};

}
