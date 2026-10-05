#include "StdinWriter.h"

#include "process.hpp"

namespace supercollidaw {

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
