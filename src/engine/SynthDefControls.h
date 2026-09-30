#pragma once

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace supercollidaw {

struct SynthDefControls {
    std::string name;
    std::set<uint32_t> controlBusesRead;
};

// Finds the control buses each SynthDef reads with In.kr(<constant>), from a .scsyndef (SCgf v0 to v2) file.
std::optional<std::vector<SynthDefControls>> scanSynthDefs(std::string_view scgf);

}
