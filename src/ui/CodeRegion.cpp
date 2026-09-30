#include "CodeRegion.h"

#include <optional>

namespace supercollidaw {

namespace {

class ParenthesisScanner {
public:
    bool closesIn(const std::string& line) {
        mClosed = false;
        for (size_t i = 0; i < line.size() && !mClosed;)
            i = scan(line, i);
        return mClosed;
    }

private:
    enum class State { code, blockComment, doubleQuoted, singleQuoted };

    size_t scan(const std::string& line, size_t i) {
        const char c = line[i];
        const char next = i + 1 < line.size() ? line[i + 1] : '\0';
        switch (mState) {
        case State::code:
            return scanCode(line, i, c, next);
        case State::blockComment:
            return scanBlockComment(i, c, next);
        case State::doubleQuoted:
            return scanQuoted(i, c, '"');
        case State::singleQuoted:
            return scanQuoted(i, c, '\'');
        }
        return i + 1;
    }

    size_t scanCode(const std::string& line, size_t i, char c, char next) {
        if (c == '/' && next == '/')
            return line.size();
        if (c == '/' && next == '*')
            return enter(State::blockComment, i + 2);
        if (c == '$')
            return i + (next == '\\' ? 3 : 2);
        if (c == '"')
            return enter(State::doubleQuoted, i + 1);
        if (c == '\'')
            return enter(State::singleQuoted, i + 1);
        if (c == '(')
            ++mDepth;
        if (c == ')')
            mClosed = --mDepth == 0;
        return i + 1;
    }

    size_t scanBlockComment(size_t i, char c, char next) {
        return c == '*' && next == '/' ? enter(State::code, i + 2) : i + 1;
    }

    size_t scanQuoted(size_t i, char c, char quote) {
        if (c == '\\')
            return i + 2;
        return c == quote ? enter(State::code, i + 1) : i + 1;
    }

    size_t enter(State state, size_t next) {
        mState = state;
        return next;
    }

    State mState = State::code;
    int mDepth = 0;
    bool mClosed = false;
};

bool startsWith(const std::string& line, char c) { return !line.empty() && line[0] == c; }

std::optional<size_t> closingLine(const std::vector<std::string>& lines, size_t openingLine) {
    ParenthesisScanner scanner;
    for (size_t line = openingLine; line < lines.size(); ++line) {
        if (scanner.closesIn(lines[line]))
            return line;
    }
    return std::nullopt;
}

}

LineRange regionAround(const std::vector<std::string>& lines, size_t cursorLine) {
    for (size_t start = cursorLine + 1; start-- > 0;) {
        if (!startsWith(lines[start], '('))
            continue;
        const std::optional<size_t> close = closingLine(lines, start);
        if (close && *close >= cursorLine && startsWith(lines[*close], ')'))
            return { start, *close };
    }
    return { cursorLine, cursorLine };
}

}
