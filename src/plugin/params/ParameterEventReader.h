#pragma once

#include "ParameterBank.h"
#include "engine/Engine.h"

#include <clap/clap.h>

namespace supercollidaw {

class ParameterEventReader : public ControlSource {
public:
    ParameterEventReader(ParameterBank& bank, const clap_input_events* events);

    void writeControls(uint32_t frame, float* buses, uint32_t numBuses) override;
    void applyAll() { applyUntil(UINT32_MAX); }

private:
    void applyUntil(uint32_t frame);
    void apply(const clap_event_header& header);

    ParameterBank& mBank;
    const clap_input_events* mEvents;
    uint32_t mCount;
    uint32_t mNext = 0;
};

}
