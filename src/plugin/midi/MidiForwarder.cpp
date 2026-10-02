#include "MidiForwarder.h"

#include "scsynthsend.h"

namespace supercollidaw {

namespace asio = boost::asio;
using asio::ip::udp;

namespace {

constexpr size_t kQueueCapacity = 1024;
constexpr char kAddress[] = "/supercollidaw/midi";

}

MidiForwarder::MidiForwarder():
    mQueue(kQueueCapacity), mSocket(mIo, udp::endpoint(asio::ip::address_v4::loopback(), 0)), mThread([this] { run(); }) {}

MidiForwarder::~MidiForwarder() {
    mStopping = true;
    mWake.release();
    mThread.join();
}

void MidiForwarder::forward(const clap_input_events* events) {
    const uint32_t count = events ? events->size(events) : 0;
    bool pushed = false;
    for (uint32_t index = 0; index < count; ++index)
        pushed = push(*events->get(events, index)) || pushed;
    if (pushed)
        mWake.release();
}

bool MidiForwarder::push(const clap_event_header& header) {
    if (header.space_id != CLAP_CORE_EVENT_SPACE_ID || header.type != CLAP_EVENT_MIDI)
        return false;
    const auto& event = reinterpret_cast<const clap_event_midi&>(header);
    return mQueue.push({ event.data[0], event.data[1], event.data[2] });
}

void MidiForwarder::run() {
    for (mWake.acquire(); !mStopping; mWake.acquire()) {
        const uint16_t port = mSclangPort;
        mQueue.consume_all([&](const Message& message) { send(message, port); });
    }
}

void MidiForwarder::send(const Message& message, uint16_t port) {
    if (port == 0)
        return;
    small_scpacket packet;
    packet.adds(kAddress);
    packet.maketags(4);
    packet.addtag(',');
    packet.addtag('i');
    packet.addi(message.status);
    packet.addtag('i');
    packet.addi(message.data1);
    packet.addtag('i');
    packet.addi(message.data2);
    boost::system::error_code ignored;
    mSocket.send_to(asio::buffer(packet.data(), packet.size()), udp::endpoint(asio::ip::address_v4::loopback(), port), 0, ignored);
}

}
