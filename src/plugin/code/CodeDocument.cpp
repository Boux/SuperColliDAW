#include "CodeDocument.h"

namespace supercollidaw {

bool CodeDocument::link(const std::filesystem::path& path) {
    LinkedFile file(path);
    std::optional<std::string> text = file.read();
    if (!text)
        return false;
    mText = *text;
    mSavedText = std::move(*text);
    mFile = std::move(file);
    return true;
}

bool CodeDocument::save() {
    if (!mFile || !mFile->write(mText))
        return false;
    mSavedText = mText;
    return true;
}

bool CodeDocument::saveAs(const std::filesystem::path& path) {
    LinkedFile file(path);
    if (!file.write(mText))
        return false;
    mSavedText = mText;
    mFile = std::move(file);
    return true;
}

CodeDocument::FileChange CodeDocument::reloadIfChanged() {
    if (!mFile || !mFile->changedSinceRead())
        return FileChange::none;
    if (isDirty()) {
        mFile->markChangeSeen();
        return FileChange::conflict;
    }
    if (link(mFile->path()))
        return FileChange::reloaded;
    mFile->markChangeSeen();
    return FileChange::none;
}

PluginState CodeDocument::state() const { return { mText, linkedPath().string() }; }

bool CodeDocument::restore(const PluginState& state) {
    mText = state.code;
    mFile.reset();
    return state.linkedPath.empty() || link(state.linkedPath);
}

}
