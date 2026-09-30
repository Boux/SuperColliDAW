#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace supercollidaw {

class PostLog {
public:
    static constexpr size_t kMaxLines = 5000;

    void append(const std::string& line);
    void collect(uint64_t& next, std::vector<std::string>& out) const;

private:
    mutable std::mutex mMutex;
    std::deque<std::string> mLines;
    uint64_t mFirstIndex = 0;
};

}
