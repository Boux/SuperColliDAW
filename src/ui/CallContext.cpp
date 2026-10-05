#include "CallContext.h"

#include "CodeScanner.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace supercollidaw {

namespace {

struct OpenBracket {
    char bracket;
    size_t line;
    size_t column;
    size_t commas = 0;
    size_t argumentLine;
    size_t argumentColumn;
};

bool isIdentifier(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

bool isOpening(char c) { return c == '(' || c == '[' || c == '{'; }

bool isClosing(char c) { return c == ')' || c == ']' || c == '}'; }

class BracketTracker {
public:
    void track(size_t line, size_t column, char c) {
        if (isOpening(c))
            mOpen.push_back({ c, line, column, 0, line, column + 1 });
        if (isClosing(c) && !mOpen.empty())
            mOpen.pop_back();
        if (c == ',' && !mOpen.empty())
            nextArgument(mOpen.back(), line, column + 1);
    }

    // An array argument keeps the call around it, a function body starts a new scope.
    const OpenBracket* innermostCall() const {
        const auto inner = std::find_if(mOpen.rbegin(), mOpen.rend(), [](const OpenBracket& open) { return open.bracket != '['; });
        return inner != mOpen.rend() && inner->bracket == '(' ? &*inner : nullptr;
    }

private:
    static void nextArgument(OpenBracket& open, size_t line, size_t column) {
        ++open.commas;
        open.argumentLine = line;
        open.argumentColumn = column;
    }

    std::vector<OpenBracket> mOpen;
};

std::string_view trimRight(std::string_view text) {
    const size_t end = text.find_last_not_of(" \t");
    return end == std::string_view::npos ? std::string_view() : text.substr(0, end + 1);
}

std::string textFrom(const std::vector<std::string>& lines, size_t line, size_t column) {
    std::string text = lines[line].substr(std::min(column, lines[line].size()));
    for (size_t next = line + 1; next < lines.size(); ++next)
        text += "\n" + lines[next];
    return text;
}

std::string keywordAtStart(std::string_view argument) {
    const size_t start = argument.find_first_not_of(" \t\n");
    if (start == std::string_view::npos || !std::islower(static_cast<unsigned char>(argument[start])))
        return {};
    const size_t end = std::find_if_not(argument.begin() + start, argument.end(), isIdentifier) - argument.begin();
    if (end == argument.size() || argument[end] != ':')
        return {};
    return std::string(argument.substr(start, end - start));
}

}

std::optional<CallContext> enclosingCall(const std::vector<std::string>& linesBeforeCursor) {
    CodeScanner scanner;
    BracketTracker brackets;
    for (size_t line = 0; line < linesBeforeCursor.size(); ++line) {
        scanner.scanLine(linesBeforeCursor[line], [&brackets, line](size_t column, char c) {
            brackets.track(line, column, c);
            return true;
        });
    }
    const OpenBracket* call = brackets.innermostCall();
    if (!call)
        return std::nullopt;
    const std::string_view callee = trimRight(std::string_view(linesBeforeCursor[call->line]).substr(0, call->column));
    if (callee.empty() || !isIdentifier(callee.back()))
        return std::nullopt;
    return CallContext{ std::string(callee), call->commas, keywordAtStart(textFrom(linesBeforeCursor, call->argumentLine, call->argumentColumn)) };
}

}
