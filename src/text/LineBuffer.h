#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>

namespace supercollidaw {

class LineBuffer {
public:
    using LineHandler = std::function<void(const std::string& line)>;

    explicit LineBuffer(LineHandler onLine): mOnLine(std::move(onLine)) {}

    void feed(const char* bytes, size_t size);

private:
    LineHandler mOnLine;
    std::string mPartial;
};

}
