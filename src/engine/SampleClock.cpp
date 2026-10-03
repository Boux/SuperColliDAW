#include "SampleClock.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace supercollidaw {

namespace {

constexpr double kBandwidthHz = 0.1;
constexpr double kMaxOmega = 0.5;
constexpr double kMinResetSeconds = 0.01;
constexpr double kResetPeriods = 2.0;
constexpr double kMaxPeriodDeviation = 0.001;
constexpr double kOscUnitsPerSecond = 4294967296.0;

}

SampleClock::SampleClock(double sampleRate): mNominalPeriod(1.0 / sampleRate), mPeriod(mNominalPeriod) {}

// A second-order delay-locked loop, as JACK uses to filter its callback times.
void SampleClock::update(uint64_t sample, int64_t oscTimeNow) {
    if (!mStarted || sample < mSample)
        return reset(sample, oscTimeNow);
    if (sample == mSample)
        return;
    const double samples = static_cast<double>(sample - mSample);
    const double predicted = mSeconds + samples * mPeriod;
    const double error = secondsSinceOrigin(oscTimeNow) - predicted;
    // A callback this far off means the host is not running in real time, e.g. a burst or an offline render.
    if (std::fabs(error) > std::max(kMinResetSeconds, kResetPeriods * samples * mNominalPeriod))
        return reset(sample, oscTimeNow);
    const double omega = std::min(2.0 * std::numbers::pi * kBandwidthHz * samples * mNominalPeriod, kMaxOmega);
    const double minPeriod = mNominalPeriod * (1.0 - kMaxPeriodDeviation);
    const double maxPeriod = mNominalPeriod * (1.0 + kMaxPeriodDeviation);
    mSeconds = predicted + std::numbers::sqrt2 * omega * error;
    mPeriod = std::clamp(mPeriod + omega * omega * error / samples, minPeriod, maxPeriod);
    mSample = sample;
}

int64_t SampleClock::oscTimeAt(uint64_t sample) const {
    const double samples = static_cast<double>(static_cast<int64_t>(sample - mSample));
    return mOrigin + std::llround((mSeconds + samples * mPeriod) * kOscUnitsPerSecond);
}

void SampleClock::reset(uint64_t sample, int64_t oscTimeNow) {
    mOrigin = oscTimeNow;
    mSample = sample;
    mSeconds = 0.0;
    mPeriod = mNominalPeriod;
    mStarted = true;
}

double SampleClock::secondsSinceOrigin(int64_t oscTime) const { return static_cast<double>(oscTime - mOrigin) / kOscUnitsPerSecond; }

}
