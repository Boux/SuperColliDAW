#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace supercollidaw {

class LinkedFile {
public:
    explicit LinkedFile(std::filesystem::path path): mPath(std::move(path)) {}

    const std::filesystem::path& path() const { return mPath; }
    std::optional<std::string> read();
    bool write(const std::string& text);
    bool changedSinceRead() const;
    void markChangeSeen() { mReadTime = modificationTime(); }

private:
    std::filesystem::file_time_type modificationTime() const;

    std::filesystem::path mPath;
    std::filesystem::file_time_type mReadTime;
};

}
