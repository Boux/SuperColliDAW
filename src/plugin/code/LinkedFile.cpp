#include "LinkedFile.h"

#include <fstream>
#include <sstream>

namespace supercollidaw {

std::optional<std::string> LinkedFile::read() {
    std::ifstream file(mPath, std::ios::binary);
    if (!file)
        return std::nullopt;
    mReadTime = modificationTime();
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

bool LinkedFile::write(const std::string& text) {
    std::ofstream file(mPath, std::ios::binary | std::ios::trunc);
    file << text;
    file.close();
    mReadTime = modificationTime();
    return static_cast<bool>(file);
}

bool LinkedFile::changedSinceRead() const { return modificationTime() != mReadTime; }

std::filesystem::file_time_type LinkedFile::modificationTime() const {
    std::error_code ec;
    const auto time = std::filesystem::last_write_time(mPath, ec);
    return ec ? std::filesystem::file_time_type::min() : time;
}

}
