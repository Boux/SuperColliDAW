#include "engine/SampleClock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

namespace {

using supercollidaw::SampleClock;

constexpr double kSampleRate = 48000.0;
constexpr uint64_t kBlock = 512;
constexpr double kOscUnitsPerSecond = 4294967296.0;
constexpr int64_t kStart = 3900000000LL << 32;
constexpr double kMaxLateness = 0.003;

int gFailures = 0;

void check(bool condition, const char* what) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", what);
    gFailures += condition ? 0 : 1;
}

int64_t oscTime(double seconds) { return kStart + std::llround(seconds * kOscUnitsPerSecond); }

double seconds(int64_t oscTime) { return static_cast<double>(oscTime - kStart) / kOscUnitsPerSecond; }

struct Host {
    double drift;
    double pauseAt = -1.0;
    double pauseSeconds = 0.0;
    double burstUntil = -1.0;
    std::mt19937 random{ 7 };
    std::uniform_real_distribution<double> lateness{ 0.0, kMaxLateness };

    double trueTime(uint64_t sample) const {
        const double audio = static_cast<double>(sample) / kSampleRate * (1.0 + drift);
        if (audio < burstUntil)
            return audio * 0.01;
        if (burstUntil >= 0.0)
            return audio - burstUntil * 0.99;
        return pauseAt >= 0.0 && audio >= pauseAt ? audio + pauseSeconds : audio;
    }
};

double worstError(SampleClock& clock, Host& host, double fromSeconds, double toSeconds) {
    double worst = 0.0;
    for (uint64_t sample = 0; sample < static_cast<uint64_t>(toSeconds * kSampleRate); sample += kBlock) {
        clock.update(sample, oscTime(host.trueTime(sample) + host.lateness(host.random)));
        const double expected = host.trueTime(sample) + kMaxLateness / 2.0;
        if (static_cast<double>(sample) / kSampleRate >= fromSeconds)
            worst = std::max(worst, std::fabs(seconds(clock.oscTimeAt(sample)) - expected));
    }
    return worst;
}

}

int main() {
    Host steady{ 0.0 };
    SampleClock steadyClock(kSampleRate);
    const double steadyError = worstError(steadyClock, steady, 15.0, 45.0);
    std::printf("  callbacks up to %.1f ms late, smoothed error %.3f ms\n", kMaxLateness * 1000.0, steadyError * 1000.0);
    check(steadyError < 0.0003, "the clock smooths callback lateness to under 0.3 ms");

    Host drifting{ 100e-6 };
    SampleClock driftingClock(kSampleRate);
    const double driftError = worstError(driftingClock, drifting, 30.0, 90.0);
    std::printf("  audio clock 100 ppm fast, smoothed error %.3f ms\n", driftError * 1000.0);
    check(driftError < 0.0003, "the clock follows an audio clock that runs at a slightly different rate");

    Host pausing{ 0.0, 30.0, 1.0 };
    SampleClock pausingClock(kSampleRate);
    const double pauseError = worstError(pausingClock, pausing, 30.1, 40.0);
    std::printf("  after the host pauses for 1 s, error %.3f ms\n", pauseError * 1000.0);
    check(pauseError < 0.002, "the clock restarts from the wall clock when the host pauses");

    Host bursting{ 0.0 };
    bursting.burstUntil = 5.0;
    SampleClock burstingClock(kSampleRate);
    const double burstError = worstError(burstingClock, bursting, 5.0, 15.0);
    std::printf("  after 5 s of audio processed 100 times faster than real time, error %.3f ms\n", burstError * 1000.0);
    check(burstError < 0.004, "the clock is right as soon as the host goes back to real time after a burst");

    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
