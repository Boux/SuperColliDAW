#include "TransportBuses.h"

#include "ClapTransport.h"

namespace supercollidaw {

// While the DAW is stopped the beats keep counting at its tempo, as SuperColliDAW.clock does, so synced LFOs keep moving.
void TransportBuses::follow(const clap_event_transport* transport, uint32_t frames) {
    mBeatsAtCallbackStart += mCallbackFrames * beatsPerFrame();
    mCallbackFrames = frames;
    if (!hasTimeline(transport))
        return;
    mTempo = transport->tempo;
    if (transport->flags & CLAP_TRANSPORT_IS_PLAYING)
        mBeatsAtCallbackStart = beats(transport->song_pos_beats);
}

void TransportBuses::writeControls(uint32_t frame, float* buses, uint32_t numBuses) {
    if (numBuses < mFirstBus + 2)
        return;
    // A block's audio starts where its input started, one block before the frame at which its input filled up.
    const double blockStart = static_cast<double>(frame) - Engine::kBlockSize;
    buses[mFirstBus] = static_cast<float>(mTempo);
    buses[mFirstBus + 1] = static_cast<float>(mBeatsAtCallbackStart + blockStart * beatsPerFrame());
}

}
