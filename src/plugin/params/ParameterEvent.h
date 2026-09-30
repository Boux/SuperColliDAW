#pragma once

#include "ParameterSpec.h"

#include <cstdint>
#include <set>
#include <string>
#include <variant>

namespace supercollidaw {

struct SynthDefDefined {
    std::string name;
    std::set<uint32_t> controlBusesRead;
};

struct SynthDefFreed {
    std::string name;
};

struct ParameterDeclared {
    uint32_t index;
    std::string name;
    ParameterSpec spec;
};

struct ParametersReset {};

using ParameterEvent = std::variant<SynthDefDefined, SynthDefFreed, ParameterDeclared, ParametersReset>;

}
