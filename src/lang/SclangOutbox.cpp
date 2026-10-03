#include "SclangOutbox.h"

#include "scsynthsend.h"

namespace supercollidaw {

namespace asio = boost::asio;
using asio::ip::udp;

namespace {

constexpr size_t kQueueCapacity = 1024;

}

SclangOutbox::SclangOutbox():
    mQueue(kQueueCapacity), mSocket(mIo, udp::endpoint(asio::ip::address_v4::loopback(), 0)), mThread([this] { run(); }) {}

SclangOutbox::~SclangOutbox() {
    mStopping = true;
    mWake.release();
    mThread.join();
}

void SclangOutbox::post(const Message& message) {
    if (mQueue.push(message))
        mWake.release();
}

void SclangOutbox::run() {
    for (mWake.acquire(); !mStopping; mWake.acquire()) {
        const uint16_t port = mSclangPort;
        mQueue.consume_all([&](const Message& message) { std::visit([&](const auto& m) { send(m, port); }, message); });
    }
}

void SclangOutbox::send(const MidiMessage& message, uint16_t port) {
    small_scpacket packet;
    packet.adds("/supercollidaw/midi");
    packet.maketags(4);
    packet.addtag(',');
    packet.addtag('i');
    packet.addi(message.status);
    packet.addtag('i');
    packet.addi(message.data1);
    packet.addtag('i');
    packet.addi(message.data2);
    sendPacket(packet.data(), packet.size(), port);
}

void SclangOutbox::send(const TransportMessage& message, uint16_t port) {
    small_scpacket packet;
    packet.OpenBundle(message.oscTime);
    packet.BeginMsg();
    packet.adds("/supercollidaw/transport");
    packet.maketags(7);
    packet.addtag(',');
    packet.addtag('i');
    packet.addi(message.playing);
    packet.addtag('d');
    packet.addd(message.tempo);
    packet.addtag('d');
    packet.addd(message.songBeats);
    packet.addtag('d');
    packet.addd(message.barStartBeats);
    packet.addtag('i');
    packet.addi(message.barNumber);
    packet.addtag('d');
    packet.addd(message.beatsPerBar);
    packet.EndMsg();
    packet.CloseBundle();
    sendPacket(packet.data(), packet.size(), port);
}

void SclangOutbox::sendPacket(const char* data, size_t size, uint16_t port) {
    if (port == 0)
        return;
    boost::system::error_code ignored;
    mSocket.send_to(asio::buffer(data, size), udp::endpoint(asio::ip::address_v4::loopback(), port), 0, ignored);
}

}
