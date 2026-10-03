#pragma once

#include <cstdint>

namespace supercollidaw {

struct MidiMessage {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
};

}
