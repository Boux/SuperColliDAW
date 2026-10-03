#pragma once

#include <clap/clap.h>

namespace supercollidaw {

inline bool hasTimeline(const clap_event_transport* transport) {
    constexpr uint32_t kRequiredFlags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE;
    return transport && (transport->flags & kRequiredFlags) == kRequiredFlags;
}

inline double beats(clap_beattime time) { return static_cast<double>(time) / CLAP_BEATTIME_FACTOR; }

}
