#pragma once

#include "SC_CoreAudio.h"
#include "midi/MidiMessage.h"

#include <vector>

struct BlockMidi {
    int offset;
    supercollidaw::MidiMessage message;
};

class SC_PluginDriver : public SC_AudioDriver {
public:
    explicit SC_PluginDriver(struct World* inWorld);

    void BeginCallback(int64 bufferTime);
    void RunBlock(const float* const* inputs, int numInputs, float* const* outputs, int numOutputs);
    void EndCallback();

    void SendMidi(const supercollidaw::MidiMessage& message);
    const std::vector<BlockMidi>& MidiOut() const { return mMidiOut; }
    void ClearMidiOut();

protected:
    bool DriverSetup(int* outNumSamplesPerCallback, double* outSampleRate) override;
    bool DriverStart() override { return true; }
    bool DriverStop() override { return true; }

private:
    void PerformScheduledBundles(int64 nextTime);
    void DropMidi();

    std::vector<BlockMidi> mMidiOut;
    bool mMidiOutOverflowed = false;
};
