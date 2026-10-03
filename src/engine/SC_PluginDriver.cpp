#include "SC_PluginDriver.h"

#include "SC_HiddenWorld.h"
#include "SC_Prototypes.h"
#include "SC_Time.hpp"
#include "SC_UnitDef.h"
#include "SC_WorldOptions.h"
#include "sc_msg_iter.h"

#include <algorithm>
#include <cmath>
#include <cstring>

void sc_SetDenormalFlags();

namespace {

constexpr char kMidiOutCommand[] = "/supercollidaw/midiOut";
constexpr size_t kMaxMidiOutPerBlock = 1024;

void performMidiOut(World* world, void*, sc_msg_iter* args, void*) {
    const auto status = static_cast<uint8_t>(args->geti());
    const auto data1 = static_cast<uint8_t>(args->geti() & 0x7F);
    const auto data2 = static_cast<uint8_t>(args->geti() & 0x7F);
    static_cast<SC_PluginDriver*>(AudioDriver(world))->SendMidi({ status, data1, data2 });
}

}

int32 server_timeseed() { return timeSeed(); }

int64 oscTimeNow() { return OSCTime(getTime()); }

// World_New calls this once after each library init, which starts a new plugin command table.
void initializeScheduler() { PlugIn_DefineCmd(kMidiOutCommand, performMidiOut, nullptr); }

SC_AudioDriver* SC_NewAudioDriver(struct World* inWorld) { return new SC_PluginDriver(inWorld); }

SC_PluginDriver::SC_PluginDriver(struct World* inWorld): SC_AudioDriver(inWorld) { mMidiOut.reserve(kMaxMidiOutPerBlock); }

bool SC_PluginDriver::DriverSetup(int* outNumSamplesPerCallback, double* outSampleRate) {
    if (!mPreferredSampleRate)
        return false;
    *outSampleRate = mPreferredSampleRate;
    *outNumSamplesPerCallback = mWorld->mBufLength;
    return true;
}

void SC_PluginDriver::BeginCallback(int64 bufferTime) {
    sc_SetDenormalFlags();
    mOSCbuftime = bufferTime;
    mFromEngine.Free();
    mToEngine.Perform();
    mOscPacketsToEngine.Perform();
}

void SC_PluginDriver::RunBlock(const float* const* inputs, int numInputs, float* const* outputs, int numOutputs) {
    World* world = mWorld;
    const int bufFrames = world->mBufLength;
    const int32 bufCounter = world->mBufCounter;
    const int inCount = std::min<int>(numInputs, world->mNumInputs);
    const int outCount = std::min<int>(numOutputs, world->mNumOutputs);

    float* inBuses = world->mAudioBus + world->mNumOutputs * bufFrames;
    int32* inTouched = world->mAudioBusTouched + world->mNumOutputs;
    for (int k = 0; k < inCount; ++k) {
        std::memcpy(inBuses + k * bufFrames, inputs[k], bufFrames * sizeof(float));
        inTouched[k] = bufCounter;
    }

    const int64 nextTime = mOSCbuftime + mOSCincrement;
    try {
        PerformScheduledBundles(nextTime);
        World_Run(world);
    } catch (std::exception& exc) {
        scprintf("SC_PluginDriver: exception in real time: %s\n", exc.what());
    } catch (...) {
        scprintf("SC_PluginDriver: unknown exception in real time\n");
    }

    const float* outBuses = world->mAudioBus;
    const int32* outTouched = world->mAudioBusTouched;
    for (int k = 0; k < outCount; ++k) {
        if (outTouched[k] == bufCounter)
            std::memcpy(outputs[k], outBuses + k * bufFrames, bufFrames * sizeof(float));
        else
            std::memset(outputs[k], 0, bufFrames * sizeof(float));
    }
    for (int k = outCount; k < numOutputs; ++k)
        std::memset(outputs[k], 0, bufFrames * sizeof(float));

    world->mBufCounter++;
    mOSCbuftime = nextTime;
}

void SC_PluginDriver::PerformScheduledBundles(int64 nextTime) {
    World* world = mWorld;
    int64 schedTime;
    while ((schedTime = mScheduler.NextTime()) <= nextTime) {
        const float diffTime = (float)(schedTime - mOSCbuftime) * mOSCtoSamples + 0.5f;
        const float diffTimeFloor = std::floor(diffTime);
        world->mSampleOffset = std::clamp((int)diffTimeFloor, 0, world->mBufLength - 1);
        world->mSubsampleOffset = diffTime - diffTimeFloor;
        SC_ScheduledEvent event = mScheduler.Remove();
        event.Perform();
    }
    world->mSampleOffset = 0;
    world->mSubsampleOffset = 0.f;
}

void SC_PluginDriver::EndCallback() { mAudioSync.Signal(); }

void SC_PluginDriver::SendMidi(const supercollidaw::MidiMessage& message) {
    if (mMidiOut.size() == mMidiOut.capacity())
        return DropMidi();
    mMidiOut.push_back({ mWorld->mSampleOffset, message });
}

void SC_PluginDriver::DropMidi() {
    if (!mMidiOutOverflowed)
        scprintf("SuperColliDAW: more than %zu MIDI messages in one block, the rest are dropped\n", kMaxMidiOutPerBlock);
    mMidiOutOverflowed = true;
}

void SC_PluginDriver::ClearMidiOut() {
    mMidiOut.clear();
    mMidiOutOverflowed = false;
}
