#include "LineBuffer.h"

namespace supercollidaw {

void LineBuffer::feed(const char* bytes, size_t size) {
    mPartial.append(bytes, size);
    size_t start = 0;
    for (size_t end = mPartial.find('\n'); end != std::string::npos; end = mPartial.find('\n', start)) {
        mOnLine(mPartial.substr(start, end - start));
        start = end + 1;
    }
    mPartial.erase(0, start);
}

}
