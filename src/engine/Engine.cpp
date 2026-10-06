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
    mSampleRate(config.sampleRate),
    mOutput(config.onPost),
    mNumInputs(config.numInputs),
    mNumOutputs(config.numOutputs),
    mInStage(config.numInputs * kBlockSize, 0.f),
    mOutStage(config.numOutputs * kBlockSize, 0.f),
    mInStagePtrs(channelPointers(mInStage, config.numInputs)),
    mOutStagePtrs(channelPointers(mOutStage, config.numOutputs)),
    mClock(config.sampleRate) {
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

    ServerOutput::install();
    ServerOutput::Scope output(mOutput, ServerOutput::Thread::nonRealtime);
    mWorld = World_New(&options);
    if (!mWorld)
        return;
    mDriver = static_cast<SC_PluginDriver*>(mWorld->hw->mAudioDriver);
    mPendingMidi.reserve(mDriver->MidiOut().capacity());
}

Engine::~Engine() {
    if (!mWorld)
        return;
    {
        ServerOutput::Scope output(mOutput, ServerOutput::Thread::nonRealtime);
        World_Cleanup(mWorld, false);
    }
    mOutput.drain();
}

void Engine::process(const float* const* inputs, float* const* outputs, uint32_t numFrames, std::initializer_list<ControlSource*> controls, MidiSink& midi) {
    ServerOutput::Scope output(mOutput, ServerOutput::Thread::realtime);
    // Before BeginCallback, so the first drain binds the NRT thread before any async command stage runs there.
    if (mOutput.takeDrainRequest())
        drainOutputInNonRealtime();
    mClock.update(mSampleCount, oscTimeNow());
    mCallbackStart = mSampleCount;
    mDriver->BeginCallback(mClock.oscTimeAt(mSampleCount - mStagePos));
    writePendingMidi(numFrames, midi);
    for (uint32_t done = 0; done < numFrames;) {
        const uint32_t n = std::min(numFrames - done, kBlockSize - mStagePos);
        exchange(inputs, outputs, done, n);
        done += n;
        mStagePos += n;
        if (mStagePos < kBlockSize)
            continue;
        for (ControlSource* source : controls)
            source->writeControls(done, mWorld->mControlBus, mWorld->mNumControlBusChannels);
        mDriver->RunBlock(mInStagePtrs.data(), mNumInputs, mOutStagePtrs.data(), mNumOutputs);
        writeBlockMidi(done, numFrames, midi);
        mStagePos = 0;
    }
    mDriver->EndCallback();
    mSampleCount += numFrames;
}

void Engine::exchange(const float* const* inputs, float* const* outputs, uint32_t offset, uint32_t numFrames) {
    const size_t bytes = numFrames * sizeof(float);
    for (uint32_t ch = 0; ch < mNumInputs; ++ch)
        std::memcpy(mInStagePtrs[ch] + mStagePos, inputs[ch] + offset, bytes);
    for (uint32_t ch = 0; ch < mNumOutputs; ++ch)
        std::memcpy(outputs[ch] + offset, mOutStagePtrs[ch] + mStagePos, bytes);
}

// A block's output starts at the frame where its input filled up, which keeps its MIDI aligned with its audio.
void Engine::writeBlockMidi(uint32_t blockStart, uint32_t numFrames, MidiSink& midi) {
    for (const BlockMidi& event : mDriver->MidiOut())
        mPendingMidi.push_back({ mCallbackStart + blockStart + event.offset, event.message });
    mDriver->ClearMidiOut();
    writePendingMidi(numFrames, midi);
}

void Engine::writePendingMidi(uint32_t numFrames, MidiSink& midi) {
    const uint64_t end = mCallbackStart + numFrames;
    const auto due = std::ranges::partition_point(mPendingMidi, [end](const PendingMidi& pending) { return pending.sample < end; });
    for (const PendingMidi& pending : std::ranges::subrange(mPendingMidi.begin(), due))
        midi.writeMidi(static_cast<uint32_t>(pending.sample - mCallbackStart), pending.message);
    mPendingMidi.erase(mPendingMidi.begin(), due);
}

void Engine::drainOutputInNonRealtime() {
    FifoMsg message;
    message.Set(mWorld, ServerOutput::drainInNonRealtime, nullptr, &mOutput);
    mDriver->SendMsgFromEngine(message);
}

bool Engine::sendPacket(char* data, int size, ReplyFunc replyFunc, void* replyContext) {
    ServerOutput::Scope output(mOutput, ServerOutput::Thread::nonRealtime);
    return World_SendPacketWithContext(mWorld, size, data, replyFunc, replyContext);
}

}
