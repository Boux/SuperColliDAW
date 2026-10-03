#include "MidiInput.h"

namespace supercollidaw {

namespace {

void forwardEvent(const clap_event_header& header, SclangOutbox& outbox) {
    if (header.space_id != CLAP_CORE_EVENT_SPACE_ID || header.type != CLAP_EVENT_MIDI)
        return;
    const auto& event = reinterpret_cast<const clap_event_midi&>(header);
    outbox.post(MidiMessage{ event.data[0], event.data[1], event.data[2] });
}

}

void forwardMidi(const clap_input_events* events, SclangOutbox& outbox) {
    const uint32_t count = events ? events->size(events) : 0;
    for (uint32_t index = 0; index < count; ++index)
        forwardEvent(*events->get(events, index), outbox);
}

}
