#include "LinkedFile.h"

#include <fstream>
#include <sstream>

namespace supercollidaw {

std::string LinkedFile::read() {
    mReadTime = modificationTime();
    std::ifstream file(mPath, std::ios::binary);
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

bool LinkedFile::changedSinceRead() const { return modificationTime() != mReadTime; }

std::filesystem::file_time_type LinkedFile::modificationTime() const {
    std::error_code ec;
    const auto time = std::filesystem::last_write_time(mPath, ec);
    return ec ? std::filesystem::file_time_type::min() : time;
}

}
