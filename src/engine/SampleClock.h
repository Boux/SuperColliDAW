#pragma once

#include <cstdint>

namespace supercollidaw {

class SampleClock {
public:
    explicit SampleClock(double sampleRate);

    void update(uint64_t sample, int64_t oscTimeNow);
    int64_t oscTimeAt(uint64_t sample) const;

private:
    void reset(uint64_t sample, int64_t oscTimeNow);
    double secondsSinceOrigin(int64_t oscTime) const;

    double mNominalPeriod;
    double mPeriod;
    int64_t mOrigin = 0;
    uint64_t mSample = 0;
    double mSeconds = 0.0;
    bool mStarted = false;
};

}
