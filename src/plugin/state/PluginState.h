#pragma once

#include "plugin/params/ParameterSpec.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace supercollidaw {

struct ParameterState {
    uint32_t index = 0;
    double value = 0.0;
    bool declared = false;
    bool visible = false;
    std::string name;
    ParameterSpec spec;
};

struct PluginState {
    std::string code;
    std::string filePath;
    std::vector<ParameterState> parameters;
};

std::string encodeState(const PluginState& state);
std::optional<PluginState> decodeState(std::string_view bytes);

}
