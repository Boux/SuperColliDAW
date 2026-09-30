#pragma once

#include "code/LinkedFile.h"
#include "engine/Engine.h"
#include "engine/OscPort.h"
#include "lang/SclangProcess.h"

#include <clap/clap.h>

#include <memory>
#include <string>
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
    static void post(const std::string& line);

    bool init();
    void destroy();
    bool activate(double sampleRate, uint32_t maxFrames);
    void deactivate();
    clap_process_status process(const clap_process* process);
    const void* extension(const char* id) const;
    void onTimer(clap_id timerId);

    void startSclang();
    void runCode();

    static uint32_t audioPortCount(const clap_plugin* plugin, bool isInput);
    static bool audioPortInfo(const clap_plugin* plugin, uint32_t index, bool isInput, clap_audio_port_info* info);
    static uint32_t latency(const clap_plugin* plugin);

    static const clap_plugin_audio_ports kAudioPorts;
    static const clap_plugin_latency kLatency;
    static const clap_plugin_timer_support kTimerSupport;

    clap_plugin mClapPlugin;
    const clap_host* mHost;
    const clap_host_timer_support* mHostTimer = nullptr;
    clap_id mCodePollTimer = CLAP_INVALID_ID;
    std::unique_ptr<OscPort> mOscPort;
    std::unique_ptr<SclangProcess> mSclang;
    std::unique_ptr<LinkedFile> mCode;
    std::unique_ptr<Engine> mEngine;
    std::vector<float> mSilence;
};

}
