#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace TinyProcessLib {
class Process;
}

namespace supercollidaw {

class StdinWriter {
public:
    explicit StdinWriter(TinyProcessLib::Process& process);
    ~StdinWriter();
    StdinWriter(const StdinWriter&) = delete;
    StdinWriter& operator=(const StdinWriter&) = delete;

    void write(std::string bytes);
    void close();

private:
    static void blockBrokenPipeSignal();

    void run();
    std::optional<std::string> next();

    TinyProcessLib::Process& mProcess;
    std::mutex mMutex;
    std::condition_variable mWake;
    std::deque<std::string> mPending;
    bool mClosing = false;
    std::thread mThread;
};

}
