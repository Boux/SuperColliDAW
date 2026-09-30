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
    check(decoded && decoded->code == state.code && decoded->linkedPath == state.linkedPath, "state survives encode and decode");
    check(!decodeState("garbage"), "garbage is rejected");
    check(!decodeState(encodeState(state).substr(0, 12)), "truncated state is rejected");
}

void testLinkedDocument(const fs::path& dir) {
    const fs::path file = dir / "linked.scd";
    writeFile(file, "a");
    CodeDocument document("embedded");
    check(!document.isLinked() && !document.isDirty(), "a new document is embedded and clean");
    check(document.link(file) && document.text() == "a", "linking reads the file");

    document.edit("b");
    check(document.isDirty(), "editing a linked document makes it dirty");
    check(document.save() && readFile(file) == "b" && !document.isDirty(), "save writes the file and clears dirty");
    check(document.reloadIfChanged() == CodeDocument::FileChange::none, "our own save is not an external change");

    writeFile(file, "c");
    check(document.reloadIfChanged() == CodeDocument::FileChange::reloaded && document.text() == "c", "an external change reloads a clean document");

    document.edit("d");
    writeFile(file, "e");
    check(document.reloadIfChanged() == CodeDocument::FileChange::conflict && document.text() == "d", "an external change keeps unsaved edits");
    check(document.reloadIfChanged() == CodeDocument::FileChange::none, "a conflict is reported once");

    const PluginState state = document.state();
    CodeDocument restored("");
    check(restored.restore(state) && restored.isLinked() && restored.text() == "e", "restoring a linked state reads the file");

    fs::remove(file);
    CodeDocument missing("");
    check(!missing.restore(state) && !missing.isLinked() && missing.text() == "d", "a missing file falls back to the saved copy");
}

}

int main() {
    char dir[] = "/tmp/supercollidaw-document-XXXXXX";
    const fs::path tempDir = mkdtemp(dir);
    testStateRoundTrip();
    testLinkedDocument(tempDir);
    fs::remove_all(tempDir);
    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
