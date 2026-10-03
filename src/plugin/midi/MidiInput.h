#pragma once

#include "lang/SclangOutbox.h"

#include <clap/clap.h>

namespace supercollidaw {

void forwardMidi(const clap_input_events* events, SclangOutbox& outbox);

}
