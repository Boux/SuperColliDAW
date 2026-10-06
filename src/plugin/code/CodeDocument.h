#pragma once

#include "LinkedFile.h"
#include "plugin/state/PluginState.h"

#include <filesystem>
#include <optional>
#include <string>

namespace supercollidaw {

class CodeDocument {
public:
    explicit CodeDocument(std::string text): mText(std::move(text)) {}

    const std::string& text() const { return mText; }
    bool hasFile() const { return mFile.has_value(); }
    std::filesystem::path filePath() const { return mFile ? mFile->path() : std::filesystem::path(); }
    bool isDirty() const { return mFile && mFileText != mText; }

    void edit(std::string text) { mText = std::move(text); }
    bool open(const std::filesystem::path& path);
    bool save();
    bool saveAs(const std::filesystem::path& path);
    bool refreshFileText();

    PluginState state() const;
    void restore(const PluginState& state);

private:
    void readFileText();

    std::string mText;
    std::optional<std::string> mFileText;
    std::optional<LinkedFile> mFile;
};

}
