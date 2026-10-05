#include "CodeCompletion.h"

#include <algorithm>
#include <utility>

namespace supercollidaw {

namespace {

constexpr size_t kRecentReplies = 16;

std::string textBefore(const TextEditor& editor, TextEditor::DocPos position) { return editor.GetSectionText(TextEditor::DocPos(position.line, 0), position); }

}

CodeCompletion::CodeCompletion(TextEditor& editor, Request request): mEditor(editor), mRequest(std::move(request)) {
    TextEditor::AutoCompleteConfig config;
    config.callback = [this](TextEditor::AutoCompleteState& state) { suggest(state); };
    mEditor.SetAutoCompleteConfig(&config);
}

void CodeCompletion::update() {
    const std::string line = textBefore(mEditor, mEditor.GetMainCursorPosition());
    if (line.empty() || line == mRequested)
        return;
    mRequested = line;
    mRequest(line);
}

void CodeCompletion::show(const Completion& completion) {
    mRecent.push_front(completion);
    if (mRecent.size() > kRecentReplies)
        mRecent.pop_back();
    if (completion.line == textBefore(mEditor, mEditor.GetMainCursorPosition()))
        mEditor.SetAutoCompleteSuggestions(completion.names);
}

// The popup asks in the same frame as the keystroke, before sclang can answer, so the list shown so far narrows until the answer arrives.
void CodeCompletion::suggest(TextEditor::AutoCompleteState& state) {
    const bool sameWord = std::exchange(mWordStart, state.searchTermStart) == state.searchTermStart;
    if (const Completion* reply = recentReply(textBefore(mEditor, state.searchTermEnd))) {
        state.suggestions = reply->names;
        return;
    }
    if (!sameWord)
        state.suggestions.clear();
    std::erase_if(state.suggestions, [&state](const std::string& name) { return !name.starts_with(state.searchTerm); });
}

const Completion* CodeCompletion::recentReply(const std::string& line) const {
    const auto found = std::find_if(mRecent.begin(), mRecent.end(), [&line](const Completion& reply) { return reply.line == line; });
    return found == mRecent.end() ? nullptr : &*found;
}

}
