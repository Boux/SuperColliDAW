#include "PluginState.h"

#include <cstring>

namespace supercollidaw {

namespace {

constexpr std::string_view kMagic = "SCDW";
constexpr uint32_t kVersion = 2;
constexpr uint32_t kDeclaredFlag = 1u << 0;
constexpr uint32_t kVisibleFlag = 1u << 1;

class Writer {
public:
    void raw(std::string_view bytes) { mOut.append(bytes); }

    void uint(uint64_t value, size_t size) {
        for (size_t i = 0; i < size; ++i)
            mOut += static_cast<char>((value >> (8 * i)) & 0xff);
    }

    void uint32(uint32_t value) { uint(value, 4); }

    void float64(double value) {
        uint64_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        uint(bits, 8);
    }

    void string(std::string_view value) {
        uint32(static_cast<uint32_t>(value.size()));
        raw(value);
    }

    std::string take() { return std::move(mOut); }

private:
    std::string mOut;
};

class Reader {
public:
    explicit Reader(std::string_view bytes): mBytes(bytes) {}

    bool ok() const { return mOk; }

    bool expect(std::string_view expected) {
        mOk = mOk && mBytes.substr(mPos, expected.size()) == expected;
        mPos += mOk ? expected.size() : 0;
        return mOk;
    }

    uint64_t uint(size_t size) {
        if (!has(size))
            return 0;
        uint64_t value = 0;
        for (size_t i = 0; i < size; ++i)
            value |= static_cast<uint64_t>(static_cast<unsigned char>(mBytes[mPos + i])) << (8 * i);
        mPos += size;
        return value;
    }

    uint32_t uint32() { return static_cast<uint32_t>(uint(4)); }

    double float64() {
        const uint64_t bits = uint(8);
        double value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    std::string string() {
        const uint32_t size = uint32();
        if (!has(size))
            return {};
        std::string value(mBytes.substr(mPos, size));
        mPos += size;
        return value;
    }

private:
    bool has(size_t size) {
        mOk = mOk && mBytes.size() - mPos >= size;
        return mOk;
    }

    std::string_view mBytes;
    size_t mPos = 0;
    bool mOk = true;
};

void writeParameter(Writer& writer, const ParameterState& parameter) {
    writer.uint32(parameter.index);
    writer.float64(parameter.value);
    writer.uint32((parameter.declared ? kDeclaredFlag : 0) | (parameter.visible ? kVisibleFlag : 0));
    writer.string(parameter.name);
    writer.float64(parameter.spec.minValue);
    writer.float64(parameter.spec.maxValue);
    writer.string(parameter.spec.warp);
    writer.float64(parameter.spec.curve);
    writer.float64(parameter.spec.step);
    writer.float64(parameter.spec.defaultValue);
    writer.string(parameter.spec.units);
}

ParameterState readParameter(Reader& reader) {
    ParameterState parameter;
    parameter.index = reader.uint32();
    parameter.value = reader.float64();
    const uint32_t flags = reader.uint32();
    parameter.declared = flags & kDeclaredFlag;
    parameter.visible = flags & kVisibleFlag;
    parameter.name = reader.string();
    parameter.spec.minValue = reader.float64();
    parameter.spec.maxValue = reader.float64();
    parameter.spec.warp = reader.string();
    parameter.spec.curve = reader.float64();
    parameter.spec.step = reader.float64();
    parameter.spec.defaultValue = reader.float64();
    parameter.spec.units = reader.string();
    return parameter;
}

}

std::string encodeState(const PluginState& state) {
    Writer writer;
    writer.raw(kMagic);
    writer.uint32(kVersion);
    writer.string(state.code);
    writer.string(state.filePath);
    writer.uint32(static_cast<uint32_t>(state.parameters.size()));
    for (const ParameterState& parameter : state.parameters)
        writeParameter(writer, parameter);
    return writer.take();
}

std::optional<PluginState> decodeState(std::string_view bytes) {
    Reader reader(bytes);
    if (!reader.expect(kMagic))
        return std::nullopt;
    const uint32_t version = reader.uint32();
    if (version < 1 || version > kVersion)
        return std::nullopt;
    PluginState state{ reader.string(), reader.string(), {} };
    const uint32_t numParameters = version >= 2 ? reader.uint32() : 0;
    for (uint32_t i = 0; i < numParameters && reader.ok(); ++i)
        state.parameters.push_back(readParameter(reader));
    if (!reader.ok())
        return std::nullopt;
    return state;
}

}
