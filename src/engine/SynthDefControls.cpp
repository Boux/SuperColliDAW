#include "SynthDefControls.h"

#include <cstring>

namespace supercollidaw {

namespace {

constexpr int8_t kControlRate = 1;

class ScgfReader {
public:
    explicit ScgfReader(std::string_view bytes): mBytes(bytes) {}

    bool ok() const { return mOk; }

    int32_t int32() { return static_cast<int32_t>(bigEndian(4)); }
    int16_t int16() { return static_cast<int16_t>(bigEndian(2)); }
    int8_t int8() { return static_cast<int8_t>(bigEndian(1)); }
    int32_t count(int version) { return version >= 2 ? int32() : int16(); }

    float float32() {
        const uint32_t bits = static_cast<uint32_t>(bigEndian(4));
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    std::string pstring() {
        const size_t size = static_cast<uint8_t>(int8());
        if (!has(size))
            return {};
        std::string value(mBytes.substr(mPos, size));
        mPos += size;
        return value;
    }

    void skip(size_t size) {
        if (has(size))
            mPos += size;
    }

private:
    bool has(size_t size) {
        mOk = mOk && mBytes.size() - mPos >= size;
        return mOk;
    }

    uint64_t bigEndian(size_t size) {
        if (!has(size))
            return 0;
        uint64_t value = 0;
        for (size_t i = 0; i < size; ++i)
            value = (value << 8) | static_cast<uint8_t>(mBytes[mPos + i]);
        mPos += size;
        return value;
    }

    std::string_view mBytes;
    size_t mPos = 0;
    bool mOk = true;
};

struct UGenInput {
    int32_t ugen;
    int32_t index;
};

void readUGen(ScgfReader& reader, int version, const std::vector<float>& constants, SynthDefControls& def) {
    const std::string className = reader.pstring();
    const int8_t rate = reader.int8();
    const int32_t numInputs = reader.count(version);
    const int32_t numOutputs = reader.count(version);
    reader.int16();
    std::vector<UGenInput> inputs(std::max(numInputs, 0));
    for (UGenInput& input : inputs)
        input = { reader.count(version), reader.count(version) };
    reader.skip(std::max(numOutputs, 0));

    const bool readsConstantControlBus = className == "In" && rate == kControlRate && !inputs.empty() && inputs[0].ugen == -1;
    if (!readsConstantControlBus || inputs[0].index < 0 || static_cast<size_t>(inputs[0].index) >= constants.size())
        return;
    const float bus = constants[inputs[0].index];
    for (int32_t channel = 0; bus >= 0.f && channel < numOutputs; ++channel)
        def.controlBusesRead.insert(static_cast<uint32_t>(bus) + channel);
}

SynthDefControls readDef(ScgfReader& reader, int version) {
    SynthDefControls def{ reader.pstring(), {} };
    std::vector<float> constants(std::max(reader.count(version), 0));
    for (float& constant : constants)
        constant = reader.float32();
    const int32_t numParams = reader.count(version);
    reader.skip(4 * std::max(numParams, 0));
    const int32_t numParamNames = reader.count(version);
    for (int32_t i = 0; i < numParamNames && reader.ok(); ++i) {
        reader.pstring();
        reader.count(version);
    }
    const int32_t numUGens = reader.count(version);
    for (int32_t i = 0; i < numUGens && reader.ok(); ++i)
        readUGen(reader, version, constants, def);
    const int16_t numVariants = version >= 1 ? reader.int16() : 0;
    for (int16_t i = 0; i < numVariants && reader.ok(); ++i) {
        reader.pstring();
        reader.skip(4 * std::max(numParams, 0));
    }
    return def;
}

}

std::optional<std::vector<SynthDefControls>> scanSynthDefs(std::string_view scgf) {
    if (scgf.substr(0, 4) != "SCgf")
        return std::nullopt;
    ScgfReader reader(scgf.substr(4));
    const int32_t version = reader.int32();
    const int16_t numDefs = reader.int16();
    if (version < 0 || version > 2)
        return std::nullopt;
    std::vector<SynthDefControls> defs;
    for (int16_t i = 0; i < numDefs && reader.ok(); ++i)
        defs.push_back(readDef(reader, version));
    if (!reader.ok())
        return std::nullopt;
    return defs;
}

}
