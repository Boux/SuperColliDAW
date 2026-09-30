#pragma once

#include <filesystem>
#include <string>

namespace supercollidaw {

class LinkedFile {
public:
    explicit LinkedFile(std::filesystem::path path): mPath(std::move(path)) {}

    const std::filesystem::path& path() const { return mPath; }
    std::string read();
    bool changedSinceRead() const;

private:
    std::filesystem::file_time_type modificationTime() const;

    std::filesystem::path mPath;
    std::filesystem::file_time_type mReadTime;
};

}
