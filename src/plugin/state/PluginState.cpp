#include "PluginState.h"

#include <cstdint>

namespace supercollidaw {

namespace {

constexpr std::string_view kMagic = "SCDW";
constexpr uint32_t kVersion = 1;

void appendUint32(std::string& out, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        out += static_cast<char>((value >> shift) & 0xff);
}

void appendField(std::string& out, std::string_view field) {
    appendUint32(out, static_cast<uint32_t>(field.size()));
    out.append(field);
}

class Reader {
public:
    explicit Reader(std::string_view bytes): mBytes(bytes) {}

    bool expect(std::string_view expected) {
        if (mBytes.substr(mPos, expected.size()) != expected)
            return false;
        mPos += expected.size();
        return true;
    }

    std::optional<uint32_t> uint32() {
        if (mBytes.size() - mPos < 4)
            return std::nullopt;
        uint32_t value = 0;
        for (int i = 0; i < 4; ++i)
            value |= static_cast<uint32_t>(static_cast<unsigned char>(mBytes[mPos + i])) << (8 * i);
        mPos += 4;
        return value;
    }

    std::optional<std::string> field() {
        const std::optional<uint32_t> size = uint32();
        if (!size || mBytes.size() - mPos < *size)
            return std::nullopt;
        std::string value(mBytes.substr(mPos, *size));
        mPos += *size;
        return value;
    }

private:
    std::string_view mBytes;
    size_t mPos = 0;
};

}

std::string encodeState(const PluginState& state) {
    std::string out(kMagic);
    appendUint32(out, kVersion);
    appendField(out, state.code);
    appendField(out, state.linkedPath);
    return out;
}

std::optional<PluginState> decodeState(std::string_view bytes) {
    Reader reader(bytes);
    if (!reader.expect(kMagic) || reader.uint32() != kVersion)
        return std::nullopt;
    std::optional<std::string> code = reader.field();
    std::optional<std::string> linkedPath = reader.field();
    if (!code || !linkedPath)
        return std::nullopt;
    return PluginState{ std::move(*code), std::move(*linkedPath) };
}

}
