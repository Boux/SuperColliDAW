#pragma once

#include "midi/MidiMessage.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <atomic>
#include <cstdint>
#include <semaphore>
#include <thread>
#include <variant>

namespace supercollidaw {

struct TransportMessage {
    int64_t oscTime;
    bool playing;
    double tempo;
    double songBeats;
    double barStartBeats;
    int32_t barNumber;
    double beatsPerBar;
};

class SclangOutbox {
public:
    using Message = std::variant<MidiMessage, TransportMessage>;

    SclangOutbox();
    ~SclangOutbox();
    SclangOutbox(const SclangOutbox&) = delete;
    SclangOutbox& operator=(const SclangOutbox&) = delete;

    void setSclangPort(uint16_t port) { mSclangPort = port; }
    void post(const Message& message);

private:
    void run();
    void send(const MidiMessage& message, uint16_t port);
    void send(const TransportMessage& message, uint16_t port);
    void sendPacket(const char* data, size_t size, uint16_t port);

    boost::lockfree::spsc_queue<Message> mQueue;
    std::counting_semaphore<> mWake{ 0 };
    std::atomic<bool> mStopping = false;
    std::atomic<uint16_t> mSclangPort = 0;
    boost::asio::io_context mIo;
    boost::asio::ip::udp::socket mSocket;
    std::thread mThread;
};

}
