#include "SclangProcess.h"

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

void SclangProcess::LineBuffer::feed(const char* bytes, size_t size) {
    mPartial.append(bytes, size);
    size_t start = 0;
    for (size_t end = mPartial.find('\n'); end != std::string::npos; end = mPartial.find('\n', start)) {
        mOnLine(mPartial.substr(start, end - start));
        start = end + 1;
    }
    mPartial.erase(0, start);
}

SclangProcess::SclangProcess(Config config): mConfig(std::move(config)), mStdout(mConfig.onPost), mStderr(mConfig.onPost) {
    const std::vector<std::string> arguments = {
        mConfig.executable, "-i", "supercollidaw", "-u", std::to_string(freeUdpPort()), "--include-path", mConfig.classLibraryDir,
    };
    const auto environment = environmentWith({
        { "SUPERCOLLIDAW_SERVER_PORT", std::to_string(mConfig.serverPort) },
        { "SUPERCOLLIDAW_NUM_INPUTS", std::to_string(mConfig.numInputs) },
        { "SUPERCOLLIDAW_NUM_OUTPUTS", std::to_string(mConfig.numOutputs) },
    });
    mProcess = std::make_unique<TinyProcessLib::Process>(
        arguments, std::string(), environment, [this](const char* bytes, size_t size) { mStdout.feed(bytes, size); },
        [this](const char* bytes, size_t size) { mStderr.feed(bytes, size); }, true);
}

SclangProcess::~SclangProcess() { stop(); }

bool SclangProcess::isRunning() {
    int exitStatus;
    return mProcess->get_id() > 0 && !mProcess->try_get_exit_status(exitStatus);
}

void SclangProcess::run(const std::string& code) {
    mProcess->write("SuperColliDAW.run(" + scStringLiteral(code) + ");" + kInterpretSilently);
}

void SclangProcess::stop() {
    if (!isRunning())
        return;
    mProcess->write(std::string("0.exit;") + kInterpretSilently);
    mProcess->close_stdin();
    const auto deadline = std::chrono::steady_clock::now() + kExitTimeout;
    while (isRunning() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(kExitPollInterval);
    if (isRunning())
        mProcess->kill(true);
    mProcess->get_exit_status();
}

}
