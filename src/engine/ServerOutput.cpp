#include "ServerOutput.h"

#include "SC_FifoMsg.h"
#include "SC_WorldOptions.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <utility>

namespace supercollidaw {

namespace {

constexpr size_t kRealtimeCapacity = 1 << 16;
constexpr size_t kRealtimeMessageSize = 1024;
constexpr size_t kDrainChunkSize = 4096;

thread_local ServerOutput* tOutput = nullptr;
thread_local ServerOutput::Thread tThread = ServerOutput::Thread::nonRealtime;

}

ServerOutput::Scope::Scope(ServerOutput& output, Thread thread): mPreviousOutput(tOutput), mPreviousThread(tThread) {
    tOutput = &output;
    tThread = thread;
}

ServerOutput::Scope::~Scope() {
    tOutput = mPreviousOutput;
    tThread = mPreviousThread;
}

void ServerOutput::install() { SetPrintFunc(print); }

void ServerOutput::drainInNonRealtime(FifoMsg* message) {
    auto* output = static_cast<ServerOutput*>(message->mData);
    // This runs on the World's NRT thread, which serves only this World, so the binding stays after the call.
    tOutput = output;
    tThread = Thread::nonRealtime;
    output->drain();
}

ServerOutput::ServerOutput(LineBuffer::LineHandler onLine): mRealtimeText(kRealtimeCapacity), mLines(std::move(onLine)) {}

bool ServerOutput::takeDrainRequest() { return std::exchange(mDrainRequested, false); }

void ServerOutput::drain() {
    std::array<char, kDrainChunkSize> chunk;
    std::lock_guard lock(mMutex);
    for (size_t size = mRealtimeText.pop(chunk.data(), chunk.size()); size > 0; size = mRealtimeText.pop(chunk.data(), chunk.size()))
        mLines.feed(chunk.data(), size);
}

int ServerOutput::print(const char* format, va_list args) {
    if (!tOutput)
        return std::vfprintf(stderr, format, args);
    if (tThread == Thread::realtime)
        return tOutput->printRealtime(format, args);
    return tOutput->printNonRealtime(format, args);
}

int ServerOutput::printRealtime(const char* format, va_list args) {
    std::array<char, kRealtimeMessageSize> text;
    const int length = std::vsnprintf(text.data(), text.size(), format, args);
    const size_t size = std::min(static_cast<size_t>(std::max(length, 0)), text.size() - 1);
    if (mRealtimeText.write_available() < size)
        return length;
    mRealtimeText.push(text.data(), size);
    mDrainRequested = true;
    return length;
}

int ServerOutput::printNonRealtime(const char* format, va_list args) {
    va_list sizing;
    va_copy(sizing, args);
    const int length = std::vsnprintf(nullptr, 0, format, sizing);
    va_end(sizing);
    if (length <= 0)
        return length;
    std::string text(static_cast<size_t>(length), '\0');
    std::vsnprintf(text.data(), text.size() + 1, format, args);
    std::lock_guard lock(mMutex);
    mLines.feed(text.data(), text.size());
    return length;
}

}
