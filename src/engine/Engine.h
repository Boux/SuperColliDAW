#pragma once

#include "SampleClock.h"
#include "ServerOutput.h"
#include "midi/MidiMessage.h"

#include "SC_Reply.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

struct World;
class SC_PluginDriver;

namespace supercollidaw {

class ControlSource {
public:
    virtual ~ControlSource() = default;
    virtual void writeControls(uint32_t frame, float* buses, uint32_t numBuses) = 0;
};

class MidiSink {
public:
    virtual ~MidiSink() = default;
    virtual void writeMidi(uint32_t frame, const MidiMessage& message) = 0;
};

class Engine {
public:
    static constexpr uint32_t kBlockSize = 64;
    static constexpr uint32_t kLatency = kBlockSize;

    struct Config {
        double sampleRate;
        uint32_t numInputs;
        uint32_t numOutputs;
        std::string ugenPluginPath;
        LineBuffer::LineHandler onPost;
    };

    static void unloadPlugins();

    explicit Engine(const Config& config);
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool isRunning() const { return mWorld != nullptr; }
    double sampleRate() const { return mSampleRate; }

    void process(const float* const* inputs, float* const* outputs, uint32_t numFrames, std::initializer_list<ControlSource*> controls, MidiSink& midi);
    bool sendPacket(char* data, int size, ReplyFunc replyFunc, void* replyContext);
    int64_t oscTimeAtFrame(uint32_t frame) const { return mClock.oscTimeAt(mCallbackStart + frame); }

private:
    struct PendingMidi {
        uint64_t sample;
        MidiMessage message;
    };

    void exchange(const float* const* inputs, float* const* outputs, uint32_t offset, uint32_t numFrames);
    void writeBlockMidi(uint32_t blockStart, uint32_t numFrames, MidiSink& midi);
    void writePendingMidi(uint32_t numFrames, MidiSink& midi);
    void drainOutputInNonRealtime();

    double mSampleRate;
    ServerOutput mOutput;
    World* mWorld = nullptr;
    SC_PluginDriver* mDriver = nullptr;
    uint32_t mNumInputs;
    uint32_t mNumOutputs;
    std::vector<float> mInStage;
    std::vector<float> mOutStage;
    std::vector<float*> mInStagePtrs;
    std::vector<float*> mOutStagePtrs;
    uint32_t mStagePos = 0;
    SampleClock mClock;
    uint64_t mSampleCount = 0;
    uint64_t mCallbackStart = 0;
    std::vector<PendingMidi> mPendingMidi;
};

}
