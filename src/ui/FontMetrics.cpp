#include "FontMetrics.h"

#include <cstdint>
#include <string_view>

namespace supercollidaw {

namespace {

constexpr size_t kTableDirectoryStart = 12;
constexpr size_t kTableRecordSize = 16;

uint16_t readU16(std::span<const unsigned char> data, size_t at) { return static_cast<uint16_t>(data[at] << 8 | data[at + 1]); }

uint32_t readU32(std::span<const unsigned char> data, size_t at) { return static_cast<uint32_t>(readU16(data, at)) << 16 | readU16(data, at + 2); }

std::optional<std::span<const unsigned char>> findTable(std::span<const unsigned char> ttf, std::string_view tag) {
    if (ttf.size() < kTableDirectoryStart)
        return std::nullopt;
    const size_t count = readU16(ttf, 4);
    for (size_t record = kTableDirectoryStart; record < kTableDirectoryStart + count * kTableRecordSize; record += kTableRecordSize) {
        if (record + kTableRecordSize > ttf.size())
            return std::nullopt;
        if (std::string_view(reinterpret_cast<const char*>(&ttf[record]), 4) != tag)
            continue;
        const size_t offset = readU32(ttf, record + 8);
        const size_t length = readU32(ttf, record + 12);
        return offset + length <= ttf.size() ? std::optional(ttf.subspan(offset, length)) : std::nullopt;
    }
    return std::nullopt;
}

}

std::optional<FontMetrics> readFontMetrics(std::span<const unsigned char> ttf) {
    const std::optional<std::span<const unsigned char>> hhea = findTable(ttf, "hhea");
    const std::optional<std::span<const unsigned char>> head = findTable(ttf, "head");
    if (!hhea || !head || hhea->size() < 8 || head->size() < 20)
        return std::nullopt;
    return FontMetrics{ static_cast<int16_t>(readU16(*hhea, 4)), static_cast<int16_t>(readU16(*hhea, 6)), readU16(*head, 18) };
}

}
