#pragma once

#include "text/LineBuffer.h"

#include <boost/lockfree/spsc_queue.hpp>

#include <cstdarg>
#include <mutex>

struct FifoMsg;

namespace supercollidaw {

class ServerOutput {
public:
    enum class Thread { realtime, nonRealtime };

    class Scope {
    public:
        Scope(ServerOutput& output, Thread thread);
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        ServerOutput* mPreviousOutput;
        Thread mPreviousThread;
    };

    static void install();
    static void drainInNonRealtime(FifoMsg* message);

    explicit ServerOutput(LineBuffer::LineHandler onLine);

    bool takeDrainRequest();
    void drain();

private:
    static int print(const char* format, va_list args);

    int printRealtime(const char* format, va_list args);
    int printNonRealtime(const char* format, va_list args);

    boost::lockfree::spsc_queue<char> mRealtimeText;
    bool mDrainRequested = true;
    std::mutex mMutex;
    LineBuffer mLines;
};

}
