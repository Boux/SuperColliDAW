#pragma once

#include <functional>
#include <string_view>

namespace supercollidaw {

class CodeScanner {
public:
    using OnCode = std::function<bool(size_t column, char c)>;

    void scanLine(std::string_view line, const OnCode& onCode);

private:
    enum class State { code, blockComment, doubleQuoted, singleQuoted };

    size_t scan(std::string_view line, size_t i, const OnCode& onCode);
    size_t scanCode(std::string_view line, size_t i, char c, char next, const OnCode& onCode);
    size_t scanBlockComment(size_t i, char c, char next);
    size_t scanQuoted(size_t i, char c, char quote);
    size_t enter(State state, size_t next);

    State mState = State::code;
    bool mStopped = false;
};

}
