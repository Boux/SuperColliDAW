#include "SC_PluginDriver.h"

#include "SC_HiddenWorld.h"
#include "SC_Prototypes.h"
#include "SC_Time.hpp"
#include "SC_WorldOptions.h"

#include <algorithm>
#include <cmath>
#include <cstring>

void sc_SetDenormalFlags();

int32 server_timeseed() { return timeSeed(); }

int64 oscTimeNow() { return OSCTime(getTime()); }

void initializeScheduler() {}

SC_AudioDriver* SC_NewAudioDriver(struct World* inWorld) { return new SC_PluginDriver(inWorld); }

SC_PluginDriver::SC_PluginDriver(struct World* inWorld): SC_AudioDriver(inWorld) {}

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
