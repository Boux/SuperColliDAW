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
    void draw();

private:
    void drawToolbar();
    void drawSplitter();
    void handleShortcuts();
    std::string selectionOrRegion() const;
    std::string selectionOrLine() const;

    EditorActions mActions;
    TextEditor mEditor;
    PostWindow mPostWindow;
    float mPostHeight;
};

}
