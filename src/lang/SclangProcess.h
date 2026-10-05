#pragma once

#include "text/LineBuffer.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace TinyProcessLib {
class Process;
}

namespace supercollidaw {

class StdinWriter;

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

    uint16_t langPort() const { return mLangPort; }
    bool isRunning() { return !exitStatus(); }
    std::optional<int> exitStatus();
    void serverStarted();
    void serverStopped();
    void run(const std::string& code);
    void evaluate(const std::string& code);
    void complete(const std::string& line);
    void lookUpSignatures(const std::string& callee);
    void stopSound();

private:
    static std::vector<std::string> environmentVariables();

    void send(const std::string& expression);
    void shutdown();

    Config mConfig;
    uint16_t mLangPort;
    LineBuffer mStdout;
    LineBuffer mStderr;
    std::unique_ptr<TinyProcessLib::Process> mProcess;
    std::unique_ptr<StdinWriter> mStdin;
};

}
