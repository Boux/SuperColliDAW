#pragma once

#include <filesystem>
#include <memory>
#include <optional>

namespace pfd {
class open_file;
class save_file;
}

namespace supercollidaw {

class FileDialog {
public:
    enum class Kind { open, save };

    FileDialog(Kind kind, const std::filesystem::path& startPath);
    ~FileDialog();
    FileDialog(const FileDialog&) = delete;
    FileDialog& operator=(const FileDialog&) = delete;

    Kind kind() const { return mKind; }
    bool isDone();
    std::optional<std::filesystem::path> chosenPath();

private:
    Kind mKind;
    std::unique_ptr<pfd::open_file> mOpen;
    std::unique_ptr<pfd::save_file> mSave;
};

}
