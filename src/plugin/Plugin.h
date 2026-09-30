#pragma once

#include "engine/Engine.h"

#include <clap/clap.h>

#include <memory>
#include <vector>

namespace supercollidaw {

class Plugin {
public:
    static const clap_plugin_descriptor kDescriptor;
    static constexpr uint32_t kNumChannels = 2;

    explicit Plugin(const clap_host* host);

    const clap_plugin* clapPlugin() const { return &mClapPlugin; }

private:
    static Plugin* from(const clap_plugin* plugin);

    bool activate(double sampleRate, uint32_t maxFrames);
    void deactivate();
    clap_process_status process(const clap_process* process);
    const void* extension(const char* id) const;

    static uint32_t audioPortCount(const clap_plugin* plugin, bool isInput);
    static bool audioPortInfo(const clap_plugin* plugin, uint32_t index, bool isInput, clap_audio_port_info* info);
    static uint32_t latency(const clap_plugin* plugin);

    static const clap_plugin_audio_ports kAudioPorts;
    static const clap_plugin_latency kLatency;

    clap_plugin mClapPlugin;
    const clap_host* mHost;
    std::unique_ptr<Engine> mEngine;
    std::vector<float> mSilence;
};

}
