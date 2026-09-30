#include "CodeController.h"

#include "plugin/PluginPaths.h"

namespace supercollidaw {

namespace {

constexpr char kEmbeddedSource[] = "Embedded in the project";

}

CodeController::CodeController(std::string initialCode, Hooks hooks): mDocument(std::move(initialCode)), mHooks(std::move(hooks)) {}

EditorStatus CodeController::status() const {
    const std::string source = mDocument.isLinked() ? mDocument.linkedPath().string() : kEmbeddedSource;
    return { source, mDocument.isLinked(), mDocument.isDirty() };
}

void CodeController::edit(const std::string& code) {
    if (code == mDocument.text())
        return;
    mDocument.edit(code);
    mHooks.markProjectDirty();
    mHooks.showStatus(status());
}

void CodeController::runAll(const std::string& code) {
    edit(code);
    mHooks.run(mDocument.text());
}

void CodeController::open() { startDialog(FileDialog::Kind::open); }

void CodeController::save(const std::string& code) {
    edit(code);
    if (!mDocument.isLinked())
        return startDialog(FileDialog::Kind::save);
    if (!mDocument.save())
        mHooks.post("SuperColliDAW: could not save " + mDocument.linkedPath().string());
    mHooks.showStatus(status());
}

void CodeController::saveAs(const std::string& code) {
    edit(code);
    startDialog(FileDialog::Kind::save);
}

void CodeController::unlink(const std::string& code) {
    edit(code);
    mDocument.unlink();
    documentChanged();
}

void CodeController::poll() {
    pollDialog();
    pollLinkedFile();
}

std::string CodeController::saveState() const { return encodeState(mDocument.state()); }

bool CodeController::loadState(std::string_view bytes) {
    const std::optional<PluginState> state = decodeState(bytes);
    if (!state)
        return false;
    if (!mDocument.restore(*state))
        mHooks.post("SuperColliDAW: " + state->linkedPath + " could not be read. Using the copy saved in the project.");
    mHooks.showCode(mDocument.text());
    mHooks.showStatus(status());
    mHooks.run(mDocument.text());
    return true;
}

void CodeController::startDialog(FileDialog::Kind kind) {
    if (mDialog)
        return;
    const std::filesystem::path start = mDocument.isLinked() ? mDocument.linkedPath() : userDataDir();
    mDialog = std::make_unique<FileDialog>(kind, start);
}

void CodeController::pollDialog() {
    if (!mDialog || !mDialog->isDone())
        return;
    const std::unique_ptr<FileDialog> dialog = std::move(mDialog);
    const std::optional<std::filesystem::path> path = dialog->chosenPath();
    if (!path)
        return;
    if (dialog->kind() == FileDialog::Kind::open)
        openChosen(*path);
    else
        saveChosen(*path);
}

void CodeController::pollLinkedFile() {
    const CodeDocument::FileChange change = mDocument.reloadIfChanged();
    if (change == CodeDocument::FileChange::conflict)
        mHooks.post("SuperColliDAW: " + mDocument.linkedPath().string() + " changed on disk, but the editor has unsaved changes. "
                    "Save to overwrite the file, or open it again to discard your changes.");
    if (change != CodeDocument::FileChange::reloaded)
        return;
    documentChanged();
    mHooks.showCode(mDocument.text());
    mHooks.run(mDocument.text());
}

void CodeController::openChosen(const std::filesystem::path& path) {
    if (!mDocument.link(path)) {
        mHooks.post("SuperColliDAW: could not read " + path.string());
        return;
    }
    documentChanged();
    mHooks.showCode(mDocument.text());
    mHooks.run(mDocument.text());
}

void CodeController::saveChosen(const std::filesystem::path& path) {
    if (!mDocument.saveAs(path)) {
        mHooks.post("SuperColliDAW: could not save " + path.string());
        return;
    }
    documentChanged();
}

void CodeController::documentChanged() {
    mHooks.markProjectDirty();
    mHooks.showStatus(status());
}

}
