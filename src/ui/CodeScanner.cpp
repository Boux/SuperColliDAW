#include "CodeScanner.h"

namespace supercollidaw {

void CodeScanner::scanLine(std::string_view line, const OnCode& onCode) {
    mStopped = false;
    for (size_t i = 0; i < line.size() && !mStopped;)
        i = scan(line, i, onCode);
}

size_t CodeScanner::scan(std::string_view line, size_t i, const OnCode& onCode) {
    const char c = line[i];
    const char next = i + 1 < line.size() ? line[i + 1] : '\0';
    switch (mState) {
    case State::code:
        return scanCode(line, i, c, next, onCode);
    case State::blockComment:
        return scanBlockComment(i, c, next);
    case State::doubleQuoted:
        return scanQuoted(i, c, '"');
    case State::singleQuoted:
        return scanQuoted(i, c, '\'');
    }
    return i + 1;
}

size_t CodeScanner::scanCode(std::string_view line, size_t i, char c, char next, const OnCode& onCode) {
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
    mStopped = !onCode(i, c);
    return i + 1;
}

size_t CodeScanner::scanBlockComment(size_t i, char c, char next) { return c == '*' && next == '/' ? enter(State::code, i + 2) : i + 1; }

size_t CodeScanner::scanQuoted(size_t i, char c, char quote) {
    if (c == '\\')
        return i + 2;
    return c == quote ? enter(State::code, i + 1) : i + 1;
}

size_t CodeScanner::enter(State state, size_t next) {
    mState = state;
    return next;
}

}
