#include "StdinWriter.h"

#include "process.hpp"

#if !defined(_WIN32)
#    include <signal.h>
#endif

namespace supercollidaw {

namespace {

// Writing to a child that has exited raises SIGPIPE, which kills the whole host unless the writing thread blocks it.
void blockBrokenPipeSignal() {
#if !defined(_WIN32)
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &signals, nullptr);
#endif
}

}

StdinWriter::StdinWriter(TinyProcessLib::Process& process): mProcess(process), mThread([this] { run(); }) {}

StdinWriter::~StdinWriter() {
    close();
    mThread.join();
}

void StdinWriter::write(std::string bytes) {
    {
        std::lock_guard lock(mMutex);
        mPending.push_back(std::move(bytes));
    }
    mWake.notify_one();
}

void StdinWriter::close() {
    {
        std::lock_guard lock(mMutex);
        mClosing = true;
    }
    mWake.notify_one();
}

void StdinWriter::run() {
    blockBrokenPipeSignal();
    for (std::optional<std::string> bytes = next(); bytes; bytes = next())
        mProcess.write(bytes->data(), bytes->size());
    mProcess.close_stdin();
}

std::optional<std::string> StdinWriter::next() {
    std::unique_lock lock(mMutex);
    mWake.wait(lock, [this] { return mClosing || !mPending.empty(); });
    if (mPending.empty())
        return std::nullopt;
    std::string bytes = std::move(mPending.front());
    mPending.pop_front();
    return bytes;
}

}
