#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace supercollidaw {

struct PluginState {
    std::string code;
    std::string linkedPath;
};

std::string encodeState(const PluginState& state);
std::optional<PluginState> decodeState(std::string_view bytes);

}
