#pragma once

#include "CodeCompletion.h"
#include "CodeEditor.h"
#include "CodeFlash.h"
#include "EditorActions.h"
#include "ExamplesPanel.h"
#include "PostWindow.h"
#include "SignatureHint.h"

#include <string>
#include <vector>

namespace supercollidaw {

class EditorView {
public:
    EditorView(EditorActions actions, const PostLog& postLog, std::vector<Example> examples);

    void setCode(const std::string& code);
    void setStatus(EditorStatus status) { mStatus = std::move(status); }
    void showCompletion(const Completion& completion) { mCompletion.show(completion); }
    void showSignatureHelp(const SignatureHelp& help) { mSignatureHint.show(help); }
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
    void runAll();
    void evaluate(TextEditor& editor, const TextEditor::DocSelection& section);
    TextEditor& focusedEditor();

    EditorActions mActions;
    EditorStatus mStatus;
    CodeEditor mEditor;
    CodeCompletion mCompletion;
    SignatureHint mSignatureHint;
    PostWindow mPostWindow;
    ExamplesPanel mExamples;
    CodeFlash mFlash;
    float mPostHeight;
    float mExamplesShare;
    bool mShowExamples = false;
};

}
