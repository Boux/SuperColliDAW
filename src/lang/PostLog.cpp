#include "PostLog.h"

#include <algorithm>

namespace supercollidaw {

void PostLog::append(const std::string& line) {
    std::lock_guard lock(mMutex);
    mLines.push_back(line);
    if (mLines.size() <= kMaxLines)
        return;
    mLines.pop_front();
    ++mFirstIndex;
}

void PostLog::collect(uint64_t& next, std::vector<std::string>& out) const {
    std::lock_guard lock(mMutex);
    const uint64_t end = mFirstIndex + mLines.size();
    const uint64_t start = std::max(next, mFirstIndex);
    out.insert(out.end(), mLines.begin() + (start - mFirstIndex), mLines.end());
    next = end;
}

}
