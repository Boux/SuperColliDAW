#include "engine/ServerOutput.h"

#include "SC_FifoMsg.h"
#include "SC_WorldOptions.h"

#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace {

using supercollidaw::ServerOutput;
using Lines = std::vector<std::string>;

int gFailures = 0;

void check(bool condition, const char* what) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", what);
    gFailures += condition ? 0 : 1;
}

void drainOnAnotherThread(ServerOutput& output) {
    std::thread([&output] {
        FifoMsg message;
        message.Set(nullptr, ServerOutput::drainInNonRealtime, nullptr, &output);
        message.Perform();
        scprintf("from the draining thread\n");
    }).join();
}

void printInRealtime() {
    scprintf("Poll: %g\n", 0.5);
    scprintf("partial ");
    scprintf("line\n");
}

}

int main() {
    Lines lines;
    ServerOutput output([&lines](const std::string& line) { lines.push_back(line); });
    ServerOutput::install();

    check(output.takeDrainRequest(), "the first block asks for a drain, which binds the NRT thread");
    check(!output.takeDrainRequest(), "no drain is asked for without output");

    {
        ServerOutput::Scope scope(output, ServerOutput::Thread::realtime);
        printInRealtime();
    }
    check(lines.empty(), "realtime output waits for a drain");
    check(output.takeDrainRequest(), "realtime output asks for a drain");
    drainOnAnotherThread(output);
    check(lines == Lines{ "Poll: 0.5", "partial line", "from the draining thread" }, "a drain posts whole lines and routes the draining thread's own output");

    lines.clear();
    {
        ServerOutput::Scope scope(output, ServerOutput::Thread::nonRealtime);
        scprintf("FAILURE IN SERVER %s %s\n", "/s_new", "Group 1 not found");
    }
    check(lines == Lines{ "FAILURE IN SERVER /s_new Group 1 not found" }, "non-realtime output is posted right away");

    lines.clear();
    scprintf("printed by a thread no World owns\n");
    check(lines.empty(), "output outside a scope is not posted");

    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
