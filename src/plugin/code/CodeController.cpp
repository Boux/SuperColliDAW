#include "CodeController.h"

#include "plugin/PluginPaths.h"

namespace supercollidaw {

CodeController::CodeController(std::string initialCode, Hooks hooks): mDocument(std::move(initialCode)), mHooks(std::move(hooks)) {}

EditorStatus CodeController::status() const { return { mDocument.filePath().string(), mDocument.isDirty() }; }

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
    if (!mDocument.hasFile())
        return startDialog(FileDialog::Kind::save);
    if (!mDocument.save())
        mHooks.post("SuperColliDAW: could not save " + mDocument.filePath().string());
    mHooks.showStatus(status());
}

void CodeController::saveAs(const std::string& code) {
    edit(code);
    startDialog(FileDialog::Kind::save);
}

void CodeController::poll() {
    pollDialog();
    pollFile();
}

void CodeController::restore(const PluginState& state) {
    mDocument.restore(state);
    mHooks.showCode(mDocument.text());
    mHooks.showStatus(status());
    mHooks.run(mDocument.text());
}

void CodeController::startDialog(FileDialog::Kind kind) {
    if (mDialog)
        return;
    const std::filesystem::path start = mDocument.hasFile() ? mDocument.filePath() : userDataDir();
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

void CodeController::pollFile() {
    if (mDocument.refreshFileText())
        mHooks.showStatus(status());
}

void CodeController::openChosen(const std::filesystem::path& path) {
    if (!mDocument.open(path)) {
        mHooks.post("SuperColliDAW: could not read " + path.string());
        return;
    }
    documentChanged();
    mHooks.replaceCode(mDocument.text());
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
