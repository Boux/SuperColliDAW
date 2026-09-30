#include "FileDialog.h"

#include <portable-file-dialogs.h>

namespace supercollidaw {

namespace {

const std::vector<std::string> kFilters = { "SuperCollider code (*.scd)", "*.scd", "All files", "*" };

}

FileDialog::FileDialog(Kind kind, const std::filesystem::path& startPath): mKind(kind) {
    if (kind == Kind::open)
        mOpen = std::make_unique<pfd::open_file>("Open SuperCollider code", startPath.string(), kFilters);
    else
        mSave = std::make_unique<pfd::save_file>("Save SuperCollider code", startPath.string(), kFilters, pfd::opt::none);
}

// pfd's destructor waits for the user to close the dialog, which would block the host's main thread.
FileDialog::~FileDialog() {
    if (isDone())
        return;
    if (mOpen)
        mOpen->kill();
    if (mSave)
        mSave->kill();
}

bool FileDialog::isDone() { return mOpen ? mOpen->ready(0) : mSave->ready(0); }

std::optional<std::filesystem::path> FileDialog::chosenPath() {
    if (mOpen) {
        const std::vector<std::string> paths = mOpen->result();
        return paths.empty() ? std::nullopt : std::optional<std::filesystem::path>(paths.front());
    }
    const std::string path = mSave->result();
    return path.empty() ? std::nullopt : std::optional<std::filesystem::path>(path);
}

}
