#pragma once

#include "plugin/code/LinkedFile.h"
#include "ui/EditorSettings.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace supercollidaw {

EditorSettings parseSettings(std::string_view text);
std::string formatSettings(const EditorSettings& settings);

class SettingsFile {
public:
    explicit SettingsFile(std::filesystem::path path): mFile(std::move(path)) {}

    const std::filesystem::path& path() const { return mFile.path(); }
    EditorSettings load();
    std::optional<EditorSettings> reloadIfChanged();
    bool save(const EditorSettings& settings);

private:
    LinkedFile mFile;
};

}
