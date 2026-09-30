#include "Engine.h"

#include "SC_PluginDriver.h"
#include "SC_HiddenWorld.h"
#include "SC_WorldOptions.h"

#include <algorithm>
#include <cmath>
#include <cstring>

void World_UnloadPlugins();

namespace supercollidaw {

namespace {

// sclang adopts the maxLogins the server reports and rejects anything above 32; 1 is its own default.
constexpr uint32 kMaxLogins = 1;

std::vector<float*> channelPointers(std::vector<float>& stage, uint32_t numChannels) {
    std::vector<float*> pointers(numChannels);
    for (uint32_t ch = 0; ch < numChannels; ++ch)
        pointers[ch] = stage.data() + ch * Engine::kBlockSize;
    return pointers;
}

}

void Engine::unloadPlugins() { World_UnloadPlugins(); }

Engine::Engine(const Config& config):
    mNumInputs(config.numInputs),
    mNumOutputs(config.numOutputs),
    mInStage(config.numInputs * kBlockSize, 0.f),
    mOutStage(config.numOutputs * kBlockSize, 0.f),
    mInStagePtrs(channelPointers(mInStage, config.numInputs)),
    mOutStagePtrs(channelPointers(mOutStage, config.numOutputs)) {
    WorldOptions options;
    options.mNumInputBusChannels = config.numInputs;
    options.mNumOutputBusChannels = config.numOutputs;
    options.mBufLength = kBlockSize;
    options.mPreferredHardwareBufferFrameSize = kBlockSize;
    options.mPreferredSampleRate = static_cast<uint32>(std::lround(config.sampleRate));
    options.mMaxLogins = kMaxLogins;
    options.mLoadGraphDefs = 0;
    options.mRendezvous = false;
    options.mUGensPluginPath = config.ugenPluginPath.c_str();

    mWorld = World_New(&options);
    if (mWorld)
        mDriver = static_cast<SC_PluginDriver*>(mWorld->hw->mAudioDriver);
}

Engine::~Engine() {
    if (mWorld)
        World_Cleanup(mWorld, false);
}

void Engine::process(const float* const* inputs, float* const* outputs, uint32_t numFrames) {
    mDriver->BeginCallback();
    for (uint32_t done = 0; done < numFrames;) {
        const uint32_t n = std::min(numFrames - done, kBlockSize - mStagePos);
        exchange(inputs, outputs, done, n);
        done += n;
        mStagePos += n;
        if (mStagePos < kBlockSize)
            continue;
        mDriver->RunBlock(mInStagePtrs.data(), mNumInputs, mOutStagePtrs.data(), mNumOutputs);
        mStagePos = 0;
    }
    mDriver->EndCallback();
}

void Engine::exchange(const float* const* inputs, float* const* outputs, uint32_t offset, uint32_t numFrames) {
    const size_t bytes = numFrames * sizeof(float);
    for (uint32_t ch = 0; ch < mNumInputs; ++ch)
        std::memcpy(mInStagePtrs[ch] + mStagePos, inputs[ch] + offset, bytes);
    for (uint32_t ch = 0; ch < mNumOutputs; ++ch)
        std::memcpy(outputs[ch] + offset, mOutStagePtrs[ch] + mStagePos, bytes);
}

bool Engine::sendPacket(char* data, int size, ReplyFunc replyFunc, void* replyContext) {
    return World_SendPacketWithContext(mWorld, size, data, replyFunc, replyContext);
}

}
