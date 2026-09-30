#pragma once

#include "SC_CoreAudio.h"

class SC_PluginDriver : public SC_AudioDriver {
public:
    explicit SC_PluginDriver(struct World* inWorld);

    void BeginCallback();
    void RunBlock(const float* const* inputs, int numInputs, float* const* outputs, int numOutputs);
    void EndCallback();

protected:
    bool DriverSetup(int* outNumSamplesPerCallback, double* outSampleRate) override;
    bool DriverStart() override { return true; }
    bool DriverStop() override { return true; }

private:
    void PerformScheduledBundles(int64 nextTime);
};
