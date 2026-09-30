#pragma once

#include "LinkedFile.h"
#include "plugin/state/PluginState.h"

#include <filesystem>
#include <optional>
#include <string>

namespace supercollidaw {

class CodeDocument {
public:
    enum class FileChange { none, reloaded, conflict };

    explicit CodeDocument(std::string text): mText(std::move(text)) {}

    const std::string& text() const { return mText; }
    bool isLinked() const { return mFile.has_value(); }
    std::filesystem::path linkedPath() const { return mFile ? mFile->path() : std::filesystem::path(); }
    bool isDirty() const { return mFile && mText != mSavedText; }

    void edit(std::string text) { mText = std::move(text); }
    bool link(const std::filesystem::path& path);
    void unlink() { mFile.reset(); }
    bool save();
    bool saveAs(const std::filesystem::path& path);
    FileChange reloadIfChanged();

    PluginState state() const;
    bool restore(const PluginState& state);

private:
    std::string mText;
    std::string mSavedText;
    std::optional<LinkedFile> mFile;
};

}
