#include "OscPort.h"

#include "Engine.h"

#include "SC_ReplyImpl.hpp"
#include "scsynthsend.h"

#include <cstring>

namespace supercollidaw {

namespace asio = boost::asio;
using asio::ip::udp;

namespace {

bool isCommand(const char* data, size_t size, const char* command) {
    const size_t length = std::strlen(command);
    return size > length && std::memcmp(data, command, length + 1) == 0;
}

}

OscPort::OscPort(): mSocket(mIo, udp::endpoint(asio::ip::address_v4::loopback(), 0)) {
    startReceive();
    mThread = std::thread([this] { mIo.run(); });
}

OscPort::~OscPort() {
    mIo.stop();
    mThread.join();
}

void OscPort::attach(Engine* engine) {
    std::lock_guard lock(mEngineMutex);
    mEngine = engine;
}

void OscPort::startReceive() {
    mSocket.async_receive_from(asio::buffer(mBuffer), mSender, [this](const boost::system::error_code& error, size_t size) {
        if (error == asio::error::operation_aborted)
            return;
        if (!error)
            handlePacket(size);
        startReceive();
    });
}

void OscPort::handlePacket(size_t size) {
    if (isCommand(mBuffer.data(), size, "/quit")) {
        refuse("/quit", "the server lives inside the plugin and cannot be quit");
        return;
    }
    std::lock_guard lock(mEngineMutex);
    if (mEngine)
        mEngine->sendPacket(mBuffer.data(), static_cast<int>(size), reply, clientFor(mSender));
}

void OscPort::refuse(const char* command, const char* reason) {
    small_scpacket packet;
    packet.adds("/fail");
    packet.maketags(3);
    packet.addtag(',');
    packet.addtag('s');
    packet.adds(command);
    packet.addtag('s');
    packet.adds(reason);
    boost::system::error_code ignored;
    mSocket.send_to(asio::buffer(packet.data(), packet.size()), mSender, 0, ignored);
}

OscPort::Client* OscPort::clientFor(const udp::endpoint& endpoint) {
    std::unique_ptr<Client>& client = mClients[endpoint];
    if (!client)
        client = std::make_unique<Client>(Client{ this, endpoint });
    return client.get();
}

void OscPort::reply(ReplyAddress* address, char* data, int size) {
    const Client* client = static_cast<const Client*>(address->mReplyData);
    boost::system::error_code ignored;
    client->owner->mSocket.send_to(asio::buffer(data, size), client->endpoint, 0, ignored);
}

}
