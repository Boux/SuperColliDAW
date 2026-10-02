#include "SclangProcess.h"

#include "StdinWriter.h"
#include "process.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <chrono>
#include <thread>

// TODO(windows): read the environment with GetEnvironmentStringsW; environ is POSIX only.
extern char** environ;

namespace supercollidaw {

namespace {

constexpr char kInterpretSilently = 0x1b;
constexpr char kInterpretAndPrint = 0x0c;
constexpr auto kExitTimeout = std::chrono::seconds(3);
constexpr auto kExitPollInterval = std::chrono::milliseconds(20);

uint16_t freeUdpPort() {
    boost::asio::io_context io;
    boost::asio::ip::udp::socket socket(io, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0));
    return socket.local_endpoint().port();
}

TinyProcessLib::Process::environment_type environmentWith(const TinyProcessLib::Process::environment_type& additions) {
    TinyProcessLib::Process::environment_type environment;
    for (char** entry = environ; *entry; ++entry) {
        const std::string variable(*entry);
        const size_t separator = variable.find('=');
        if (separator != std::string::npos)
            environment[variable.substr(0, separator)] = variable.substr(separator + 1);
    }
    for (const auto& [name, value] : additions)
        environment[name] = value;
    return environment;
}

std::string scStringLiteral(const std::string& text) {
    std::string literal = "\"";
    for (char c : text) {
        if (c == kInterpretSilently || c == kInterpretAndPrint)
            continue;
        if (c == '"' || c == '\\')
            literal += '\\';
        literal += c;
    }
    return literal + "\"";
}

}

SclangProcess::SclangProcess(Config config):
    mConfig(std::move(config)), mLangPort(freeUdpPort()), mStdout(mConfig.onPost), mStderr(mConfig.onPost) {
    const std::vector<std::string> arguments = {
        mConfig.executable, "-i", "supercollidaw", "-u", std::to_string(mLangPort), "--include-path", mConfig.classLibraryDir,
    };
    const auto environment = environmentWith({
        { "SUPERCOLLIDAW_SERVER_PORT", std::to_string(mConfig.serverPort) },
        { "SUPERCOLLIDAW_NUM_INPUTS", std::to_string(mConfig.numInputs) },
        { "SUPERCOLLIDAW_NUM_OUTPUTS", std::to_string(mConfig.numOutputs) },
        { "SUPERCOLLIDAW_NUM_PARAMETERS", std::to_string(mConfig.numParameters) },
    });
    mProcess = std::make_unique<TinyProcessLib::Process>(
        arguments, std::string(), environment, [this](const char* bytes, size_t size) { mStdout.feed(bytes, size); },
        [this](const char* bytes, size_t size) { mStderr.feed(bytes, size); }, true);
    mStdin = std::make_unique<StdinWriter>(*mProcess);
}

SclangProcess::~SclangProcess() { shutdown(); }

std::optional<int> SclangProcess::exitStatus() {
    int status = 0;
    return mProcess->try_get_exit_status(status) ? std::optional<int>(status) : std::nullopt;
}

void SclangProcess::serverStarted() { send("SuperColliDAW.serverStarted"); }

void SclangProcess::serverStopped() { send("SuperColliDAW.serverStopped"); }

void SclangProcess::run(const std::string& code) { send("SuperColliDAW.run(" + scStringLiteral(code) + ")"); }

void SclangProcess::evaluate(const std::string& code) { send("SuperColliDAW.evaluate(" + scStringLiteral(code) + ")"); }

void SclangProcess::stopSound() { send("SuperColliDAW.stop"); }

void SclangProcess::send(const std::string& expression) { mStdin->write(expression + ";" + kInterpretSilently); }

void SclangProcess::shutdown() {
    if (!isRunning())
        return;
    send("0.exit");
    mStdin->close();
    const auto deadline = std::chrono::steady_clock::now() + kExitTimeout;
    while (isRunning() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(kExitPollInterval);
    if (isRunning())
        mProcess->kill(true);
    mProcess->get_exit_status();
}

}
