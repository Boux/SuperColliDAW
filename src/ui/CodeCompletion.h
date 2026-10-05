#pragma once

#include "Completion.h"

#include <TextEditor.h>

#include <deque>
#include <functional>
#include <string>

namespace supercollidaw {

class CodeCompletion {
public:
    using Request = std::function<void(const std::string& line)>;

    CodeCompletion(TextEditor& editor, Request request);
    CodeCompletion(const CodeCompletion&) = delete;
    CodeCompletion& operator=(const CodeCompletion&) = delete;

    void update();
    void show(const Completion& completion);

private:
    void suggest(TextEditor::AutoCompleteState& state);
    const Completion* recentReply(const std::string& line) const;

    TextEditor& mEditor;
    Request mRequest;
    std::string mRequested;
    std::deque<Completion> mRecent;
    TextEditor::DocPos mWordStart;
};

}
