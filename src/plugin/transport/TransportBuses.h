#pragma once

#include "engine/Engine.h"

#include <clap/clap.h>

namespace supercollidaw {

class TransportBuses : public ControlSource {
public:
    TransportBuses(double sampleRate, uint32_t firstBus): mSampleRate(sampleRate), mFirstBus(firstBus) {}

    void follow(const clap_event_transport* transport, uint32_t frames);
    void writeControls(uint32_t frame, float* buses, uint32_t numBuses) override;

private:
    double beatsPerFrame() const { return mTempo / 60.0 / mSampleRate; }

    double mSampleRate;
    uint32_t mFirstBus;
    double mTempo = 60.0;
    double mBeatsAtCallbackStart = 0.0;
    uint32_t mCallbackFrames = 0;
};

}
