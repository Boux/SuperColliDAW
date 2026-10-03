#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <array>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

struct ReplyAddress;

namespace supercollidaw {

class Engine;

class OscPort {
public:
    using Observer = std::function<bool(std::string_view packet)>;

    explicit OscPort(Observer observer);
    ~OscPort();
    OscPort(const OscPort&) = delete;
    OscPort& operator=(const OscPort&) = delete;

    uint16_t port() const { return mSocket.local_endpoint().port(); }
    void attach(Engine* engine);

private:
    struct Client {
        OscPort* owner;
        boost::asio::ip::udp::endpoint endpoint;
        std::atomic<bool> statusPending = false;
    };

    static void reply(ReplyAddress* address, char* data, int size);

    void startReceive();
    void handlePacket(size_t size);
    void refuse(const char* command, const char* reason);
    Client* clientFor(const boost::asio::ip::udp::endpoint& endpoint);

    boost::asio::io_context mIo;
    boost::asio::ip::udp::socket mSocket;
    boost::asio::ip::udp::endpoint mSender;
    std::array<char, 65536> mBuffer;
    std::map<boost::asio::ip::udp::endpoint, std::unique_ptr<Client>> mClients;
    Observer mObserver;
    std::mutex mEngineMutex;
    Engine* mEngine = nullptr;
    std::thread mThread;
};

}
