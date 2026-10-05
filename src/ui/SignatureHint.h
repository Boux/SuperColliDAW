#pragma once

#include "CallContext.h"
#include "CodeEditor.h"
#include "SignatureHelp.h"

#include <deque>
#include <functional>
#include <string>

namespace supercollidaw {

class SignatureHint {
public:
    using Request = std::function<void(const std::string& callee)>;

    SignatureHint(CodeEditor& editor, Request request): mEditor(editor), mRequest(std::move(request)) {}

    void update();
    void show(const SignatureHelp& help);

private:
    void request(const std::string& callee);
    const SignatureHelp* recentReply(const std::string& callee) const;
    void draw(const SignatureHelp& help, const CallContext& call) const;

    CodeEditor& mEditor;
    Request mRequest;
    std::string mRequested;
    std::deque<SignatureHelp> mRecent;
};

}
