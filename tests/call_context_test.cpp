#include "ui/CallContext.h"

#include <cstdio>

namespace {

using supercollidaw::CallContext;
using supercollidaw::enclosingCall;

int gFailures = 0;

void report(const char* what, bool passed, const std::optional<CallContext>& actual) {
    if (actual)
        std::printf("%s: %s (got \"%s\" argument %zu keyword \"%s\")\n", passed ? "PASS" : "FAIL", what, actual->callee.c_str(), actual->argument, actual->keyword.c_str());
    else
        std::printf("%s: %s (got no call)\n", passed ? "PASS" : "FAIL", what);
    gFailures += passed ? 0 : 1;
}

void check(const char* what, const std::vector<std::string>& lines, const char* callee, size_t argument, const char* keyword = "") {
    const std::optional<CallContext> actual = enclosingCall(lines);
    report(what, actual && actual->callee == callee && actual->argument == argument && actual->keyword == keyword, actual);
}

void checkNone(const char* what, const std::vector<std::string>& lines) {
    const std::optional<CallContext> actual = enclosingCall(lines);
    report(what, !actual, actual);
}

}

int main() {
    check("first argument right after the parenthesis", { "SuperColliDAW.kr(" }, "SuperColliDAW.kr", 0);
    check("commas move to the next argument", { "SuperColliDAW.kr(0, \\cutoff, 2" }, "SuperColliDAW.kr", 2);
    check("the callee keeps the code before it on the line", { "x = SinOsc.ar(440" }, "x = SinOsc.ar", 0);
    check("a keyword names the argument", { "SuperColliDAW.kr(0, \\cutoff, start: 50" }, "SuperColliDAW.kr", 2, "start");
    check("commas inside an array belong to the array", { "SinOsc.ar([440, 441], 0" }, "SinOsc.ar", 1);
    check("inside an array argument shows the call", { "SinOsc.ar([440, 4" }, "SinOsc.ar", 0);
    check("the innermost call wins", { "SinOsc.ar(LFNoise1.kr(1, " }, "SinOsc.ar(LFNoise1.kr", 1);
    check("a closed inner call returns to the outer one", { "SinOsc.ar(LFNoise1.kr(1), " }, "SinOsc.ar", 1);
    check("commas in strings, symbols, chars and comments are ignored", { "Pdef(\"a,b\", 'c,d', $,, /* , */ " }, "Pdef", 3);
    check("calls span lines", { "SuperColliDAW.kr(0,", "    \\cutoff,", "    high: " }, "SuperColliDAW.kr", 2, "high");
    check("a class called directly", { "Pdef(\\bass, " }, "Pdef", 1);
    checkNone("a function body starts a new scope", { "Pbind(\\freq, Pfunc({ |ev| ev" });
    checkNone("a region parenthesis is not a call", { "(", "x = 1" });
    checkNone("an event literal is not a call", { "x = (freq: 4" });
    checkNone("a closed call", { "SinOsc.ar(440)" });
    checkNone("a line comment hides the parenthesis", { "// SinOsc.ar(" });

    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
