#include "CodeRegion.h"

#include "CodeScanner.h"

#include <optional>

namespace supercollidaw {

namespace {

class ParenthesisScanner {
public:
    bool closesIn(const std::string& line) {
        bool closed = false;
        mScanner.scanLine(line, [this, &closed](size_t, char c) {
            closed = closes(c);
            return !closed;
        });
        return closed;
    }

private:
    bool closes(char c) {
        if (c == '(')
            ++mDepth;
        return c == ')' && --mDepth == 0;
    }

    CodeScanner mScanner;
    int mDepth = 0;
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
