#include "ui/CodeRegion.h"

#include <cstdio>

namespace {

using supercollidaw::LineRange;
using supercollidaw::regionAround;

int gFailures = 0;

void check(const char* what, const std::vector<std::string>& lines, size_t cursorLine, LineRange expected) {
    const LineRange actual = regionAround(lines, cursorLine);
    const bool passed = actual.first == expected.first && actual.last == expected.last;
    std::printf("%s: %s (got %zu-%zu)\n", passed ? "PASS" : "FAIL", what, actual.first, actual.last);
    gFailures += passed ? 0 : 1;
}

}

int main() {
    const std::vector<std::string> twoBlocks = {
        "(",
        "SynthDef(\\a, { Out.ar(0, SinOsc.ar) }).add;",
        ")",
        "x = 1;",
        "(",
        "Pbind(\\dur, 0.25).play;",
        ")",
    };
    check("cursor inside the first block", twoBlocks, 1, { 0, 2 });
    check("cursor on the opening line", twoBlocks, 0, { 0, 2 });
    check("cursor on the closing line", twoBlocks, 2, { 0, 2 });
    check("cursor between blocks evaluates its line", twoBlocks, 3, { 3, 3 });
    check("cursor inside the second block", twoBlocks, 5, { 4, 6 });

    const std::vector<std::string> tricky = {
        "(",
        "var s = \")\"; // )",
        "$) ; '(' ; /* ) */",
        ")",
    };
    check("parentheses in strings, symbols, chars and comments are ignored", tricky, 1, { 0, 3 });

    const std::vector<std::string> indented = {
        "  (",
        "  1 + 1",
        "  )",
    };
    check("indented parentheses are not a region", indented, 1, { 1, 1 });

    const std::vector<std::string> nested = {
        "(",
        "(",
        "a",
        ")",
        "b",
        ")",
    };
    check("innermost region wins", nested, 2, { 1, 3 });
    check("outer region when cursor is outside the inner one", nested, 4, { 0, 5 });

    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
