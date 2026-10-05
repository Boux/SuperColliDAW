#include "StdinWriter.h"

#include <signal.h>

namespace supercollidaw {

// Writing to a child that has exited raises SIGPIPE, which kills the whole host unless the writing thread blocks it.
void StdinWriter::blockBrokenPipeSignal() {
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &signals, nullptr);
}

}
