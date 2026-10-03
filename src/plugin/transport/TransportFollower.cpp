#include "TransportFollower.h"

#include <cmath>

namespace supercollidaw {

namespace {

constexpr uint32_t kRequiredFlags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE;
constexpr double kResendSeconds = 0.05;
constexpr double kJumpBeats = 0.001;

double beats(clap_beattime time) { return static_cast<double>(time) / CLAP_BEATTIME_FACTOR; }

double beatsPerBar(const clap_event_transport& transport) {
    const bool hasSignature = (transport.flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE) && transport.tsig_denom > 0;
    return hasSignature ? transport.tsig_num * 4.0 / transport.tsig_denom : 4.0;
}

TransportMessage messageFrom(const clap_event_transport& transport, int64_t oscTime) {
    const bool playing = transport.flags & CLAP_TRANSPORT_IS_PLAYING;
    return { oscTime, playing, transport.tempo, beats(transport.song_pos_beats), beats(transport.bar_start), transport.bar_number, beatsPerBar(transport) };
}

}

std::optional<TransportMessage> TransportFollower::follow(const clap_event_transport* transport, int64_t oscTime, uint32_t frames) {
    if (!transport || (transport->flags & kRequiredFlags) != kRequiredFlags)
        return std::nullopt;
    const TransportMessage current = messageFrom(*transport, oscTime);
    const bool send = changed(current) || mSamplesSinceSent >= mSampleRate * kResendSeconds;
    mExpectedBeats = current.songBeats + (current.playing ? frames / mSampleRate * current.tempo / 60.0 : 0.0);
    mSamplesSinceSent = send ? frames : mSamplesSinceSent + frames;
    if (!send)
        return std::nullopt;
    mLast = current;
    return current;
}

bool TransportFollower::changed(const TransportMessage& current) const {
    if (!mLast)
        return true;
    const bool jumped = current.playing && std::fabs(current.songBeats - mExpectedBeats) > kJumpBeats;
    return current.playing != mLast->playing || current.tempo != mLast->tempo || current.beatsPerBar != mLast->beatsPerBar || jumped;
}

}
