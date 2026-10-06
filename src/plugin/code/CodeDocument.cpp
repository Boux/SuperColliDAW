#include "CodeDocument.h"

namespace supercollidaw {

bool CodeDocument::open(const std::filesystem::path& path) {
    LinkedFile file(path);
    std::optional<std::string> text = file.read();
    if (!text)
        return false;
    mText = *text;
    mFileText = std::move(text);
    mFile = std::move(file);
    return true;
}

bool CodeDocument::save() {
    if (!mFile || !mFile->write(mText))
        return false;
    mFileText = mText;
    return true;
}

bool CodeDocument::saveAs(const std::filesystem::path& path) {
    LinkedFile file(path);
    if (!file.write(mText))
        return false;
    mFileText = mText;
    mFile = std::move(file);
    return true;
}

bool CodeDocument::refreshFileText() {
    if (!mFile || !mFile->changedSinceRead())
        return false;
    readFileText();
    return true;
}

PluginState CodeDocument::state() const { return { mText, filePath().string() }; }

void CodeDocument::restore(const PluginState& state) {
    mText = state.code;
    mFile.reset();
    mFileText.reset();
    if (state.filePath.empty())
        return;
    mFile.emplace(state.filePath);
    readFileText();
}

// The file's text is only compared with the code, so other instances saving it never change this one.
void CodeDocument::readFileText() {
    mFileText = mFile->read();
    if (!mFileText)
        mFile->markChangeSeen();
}

}
