#include "engine/SynthDefControls.h"

#include <cstdio>
#include <fstream>
#include <iterator>

int main(int argc, char** argv) {
    std::ifstream file(argv[1], std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const auto defs = supercollidaw::scanSynthDefs(bytes);
    int failures = 0;
    auto check = [&](bool condition, const char* what) {
        std::printf("%s: %s\n", condition ? "PASS" : "FAIL", what);
        failures += condition ? 0 : 1;
    };
    check(defs && defs->size() == 1 && defs->front().name == "params", "the SynthDef is read");
    check(defs && defs->front().controlBusesRead == std::set<uint32_t>{ 0, 3, 4 },
        "In.kr with constant buses is found, In.kr(control) and In.ar are ignored");
    check(!supercollidaw::scanSynthDefs("SCgf\0\0\0\2\0\1\3ab"), "a truncated file is rejected");
    check(!supercollidaw::scanSynthDefs("nope"), "a non-SynthDef is rejected");
    std::printf("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
