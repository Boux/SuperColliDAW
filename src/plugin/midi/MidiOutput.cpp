#include "MidiOutput.h"

namespace supercollidaw {

namespace {

constexpr uint8_t kNoteOff = 0x80;
constexpr uint8_t kNoteOn = 0x90;
constexpr uint8_t kControlChange = 0xB0;
constexpr uint8_t kAllNotesOff = 123;
constexpr uint8_t kChannels = 16;
constexpr uint8_t kKeys = 128;
constexpr uint16_t kNoteOutputPortIndex = 0;

uint8_t kind(const MidiMessage& message) { return message.status & 0xF0; }

uint8_t channel(const MidiMessage& message) { return message.status & 0x0F; }

bool isNoteOn(const MidiMessage& message) { return kind(message) == kNoteOn && message.data2 > 0; }

bool isNoteOff(const MidiMessage& message) { return kind(message) == kNoteOff || (kind(message) == kNoteOn && message.data2 == 0); }

bool isAllNotesOff(const MidiMessage& message) { return kind(message) == kControlChange && message.data1 == kAllNotesOff; }

}

// Not every instrument honours All Notes Off, so it also releases each note this plugin still holds.
void MidiOutput::writeMidi(uint32_t frame, const MidiMessage& message) {
    if (isAllNotesOff(message))
        releaseChannel(frame, channel(message));
    if (push(frame, message))
        track(message);
}

void MidiOutput::releaseHeldNotes(uint32_t frame) {
    for (uint8_t ch = 0; ch < kChannels; ++ch)
        releaseChannel(frame, ch);
}

void MidiOutput::releaseChannel(uint32_t frame, uint8_t channel) {
    for (uint8_t key = 0; key < kKeys; ++key)
        releaseKey(frame, channel, key);
}

void MidiOutput::releaseKey(uint32_t frame, uint8_t channel, uint8_t key) {
    for (uint8_t& held = mHeld[channel][key]; held > 0; --held)
        push(frame, { static_cast<uint8_t>(kNoteOff | channel), key, 0 });
}

void MidiOutput::track(const MidiMessage& message) {
    uint8_t& held = mHeld[channel(message)][message.data1 & 0x7F];
    if (isNoteOn(message) && held < UINT8_MAX)
        ++held;
    if (isNoteOff(message) && held > 0)
        --held;
}

bool MidiOutput::push(uint32_t frame, const MidiMessage& message) {
    const clap_event_midi event = {
        .header = { .size = sizeof(clap_event_midi), .time = frame, .space_id = CLAP_CORE_EVENT_SPACE_ID, .type = CLAP_EVENT_MIDI, .flags = 0 },
        .port_index = kNoteOutputPortIndex,
        .data = { message.status, message.data1, message.data2 },
    };
    return mEvents->try_push(mEvents, &event.header);
}

}
