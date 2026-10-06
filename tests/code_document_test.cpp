#include "plugin/code/CodeDocument.h"
#include "plugin/state/PluginState.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {

using supercollidaw::CodeDocument;
using supercollidaw::decodeState;
using supercollidaw::encodeState;
using supercollidaw::PluginState;

namespace fs = std::filesystem;

int gFailures = 0;

void check(bool condition, const char* what) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", what);
    gFailures += condition ? 0 : 1;
}

void writeFile(const fs::path& path, const std::string& text) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::ofstream(path, std::ios::binary) << text;
}

std::string readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void testStateRoundTrip() {
    const PluginState state{ std::string("{ SinOsc.ar }.play;\n\"quoted\" \\ ") + '\0' + "binary", "/tmp/some file.scd" };
    const std::optional<PluginState> decoded = decodeState(encodeState(state));
    check(decoded && decoded->code == state.code && decoded->filePath == state.filePath, "state survives encode and decode");
    check(!decodeState("garbage"), "garbage is rejected");
    check(!decodeState(encodeState(state).substr(0, 12)), "truncated state is rejected");
}

void testDocumentWithFile(const fs::path& dir) {
    const fs::path file = dir / "opened.scd";
    writeFile(file, "a");
    CodeDocument document("embedded");
    check(!document.hasFile() && !document.isDirty(), "a new document has no file and is clean");
    check(document.open(file) && document.text() == "a" && !document.isDirty(), "opening reads the file");

    document.edit("b");
    check(document.isDirty(), "editing makes the document differ from its file");
    check(document.save() && readFile(file) == "b" && !document.isDirty(), "save writes the file");
    check(!document.refreshFileText(), "our own save is not an external change");

    writeFile(file, "c");
    check(document.refreshFileText() && document.text() == "b" && document.isDirty(), "an external change never replaces the code");

    const PluginState state = document.state();
    CodeDocument restored("");
    restored.restore(state);
    check(restored.text() == "b" && restored.filePath() == file && restored.isDirty(), "restoring keeps the project's code and the file path");
    check(restored.save() && readFile(file) == "b" && !restored.isDirty(), "a restored document saves to its file");

    fs::remove(file);
    CodeDocument missing("");
    missing.restore(state);
    check(missing.text() == "b" && missing.hasFile() && missing.isDirty(), "a missing file keeps the path for the next save");
}

}

int main() {
    char dir[] = "/tmp/supercollidaw-document-XXXXXX";
    const fs::path tempDir = mkdtemp(dir);
    testStateRoundTrip();
    testDocumentWithFile(tempDir);
    fs::remove_all(tempDir);
    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
