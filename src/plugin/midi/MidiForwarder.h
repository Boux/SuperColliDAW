#pragma once

#include <clap/clap.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <atomic>
#include <cstdint>
#include <semaphore>
#include <thread>

namespace supercollidaw {

class MidiForwarder {
public:
    MidiForwarder();
    ~MidiForwarder();
    MidiForwarder(const MidiForwarder&) = delete;
    MidiForwarder& operator=(const MidiForwarder&) = delete;

    void setSclangPort(uint16_t port) { mSclangPort = port; }
    void forward(const clap_input_events* events);

private:
    struct Message {
        uint8_t status;
        uint8_t data1;
        uint8_t data2;
    };

    bool push(const clap_event_header& header);
    void run();
    void send(const Message& message, uint16_t port);

    boost::lockfree::spsc_queue<Message> mQueue;
    std::counting_semaphore<> mWake{ 0 };
    std::atomic<bool> mStopping = false;
    std::atomic<uint16_t> mSclangPort = 0;
    boost::asio::io_context mIo;
    boost::asio::ip::udp::socket mSocket;
    std::thread mThread;
};

}
