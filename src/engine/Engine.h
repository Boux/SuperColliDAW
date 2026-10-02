#pragma once

#include "ServerOutput.h"

#include "SC_Reply.h"

#include <cstdint>
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

    void process(const float* const* inputs, float* const* outputs, uint32_t numFrames, ControlSource& controls);
    bool sendPacket(char* data, int size, ReplyFunc replyFunc, void* replyContext);

private:
    void exchange(const float* const* inputs, float* const* outputs, uint32_t offset, uint32_t numFrames);
    void drainOutputInNonRealtime();

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
};

}
