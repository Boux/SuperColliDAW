#include "ParameterEventReader.h"

#include <algorithm>

namespace supercollidaw {

ParameterEventReader::ParameterEventReader(ParameterBank& bank, const clap_input_events* events):
    mBank(bank), mEvents(events), mCount(events ? events->size(events) : 0) {}

void ParameterEventReader::writeControls(uint32_t frame, float* buses, uint32_t numBuses) {
    applyUntil(frame);
    const uint32_t count = std::min(numBuses, ParameterBank::kCount);
    for (uint32_t index = 0; index < count; ++index)
        buses[index] = mBank.effectiveValue(index);
}

void ParameterEventReader::applyUntil(uint32_t frame) {
    for (; mNext < mCount; ++mNext) {
        const clap_event_header* header = mEvents->get(mEvents, mNext);
        if (header->time >= frame)
            return;
        apply(*header);
    }
}

void ParameterEventReader::apply(const clap_event_header& header) {
    if (header.space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;
    if (header.type == CLAP_EVENT_PARAM_VALUE) {
        const auto& event = reinterpret_cast<const clap_event_param_value&>(header);
        mBank.setValue(event.param_id, event.value);
    }
    if (header.type == CLAP_EVENT_PARAM_MOD) {
        const auto& event = reinterpret_cast<const clap_event_param_mod&>(header);
        mBank.setModulation(event.param_id, event.amount);
    }
}

}
