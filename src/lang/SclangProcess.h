#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace TinyProcessLib {
class Process;
}

namespace supercollidaw {

class SclangProcess {
public:
    using PostHandler = std::function<void(const std::string& line)>;

    struct Config {
        std::string executable;
        std::string classLibraryDir;
        uint16_t serverPort;
        uint32_t numInputs;
        uint32_t numOutputs;
        uint32_t numParameters;
        PostHandler onPost;
    };

    explicit SclangProcess(Config config);
    ~SclangProcess();
    SclangProcess(const SclangProcess&) = delete;
    SclangProcess& operator=(const SclangProcess&) = delete;

    bool isRunning();
    void run(const std::string& code);
    void evaluate(const std::string& code);
    void stopSound();

private:
    class LineBuffer {
    public:
        explicit LineBuffer(const PostHandler& onLine): mOnLine(onLine) {}
        void feed(const char* bytes, size_t size);

    private:
        const PostHandler& mOnLine;
        std::string mPartial;
    };

    void send(const std::string& expression);
    void shutdown();

    Config mConfig;
    LineBuffer mStdout;
    LineBuffer mStderr;
    std::unique_ptr<TinyProcessLib::Process> mProcess;
};

}
