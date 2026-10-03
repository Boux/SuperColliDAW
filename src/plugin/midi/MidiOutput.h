#pragma once

#include "engine/Engine.h"

#include <clap/clap.h>

#include <array>

namespace supercollidaw {

using HeldNotes = std::array<std::array<uint8_t, 128>, 16>;

class MidiOutput : public MidiSink {
public:
    MidiOutput(HeldNotes& held, const clap_output_events* events): mHeld(held), mEvents(events) {}

    void writeMidi(uint32_t frame, const MidiMessage& message) override;
    void releaseHeldNotes(uint32_t frame);

private:
    void releaseChannel(uint32_t frame, uint8_t channel);
    void releaseKey(uint32_t frame, uint8_t channel, uint8_t key);
    void track(const MidiMessage& message);
    bool push(uint32_t frame, const MidiMessage& message);

    HeldNotes& mHeld;
    const clap_output_events* mEvents;
};

}
