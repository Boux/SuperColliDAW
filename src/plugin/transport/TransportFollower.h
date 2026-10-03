#pragma once

#include "lang/SclangOutbox.h"

#include <clap/clap.h>

#include <cstdint>
#include <optional>

namespace supercollidaw {

class TransportFollower {
public:
    explicit TransportFollower(double sampleRate): mSampleRate(sampleRate) {}

    std::optional<TransportMessage> follow(const clap_event_transport* transport, int64_t oscTime, uint32_t frames);

private:
    bool changed(const TransportMessage& current) const;

    double mSampleRate;
    std::optional<TransportMessage> mLast;
    double mExpectedBeats = 0.0;
    double mSamplesSinceSent = 0.0;
};

}
