#pragma once

#include "PluginGui.h"
#include "code/CodeController.h"
#include "engine/Engine.h"
#include "engine/OscPort.h"
#include "lang/PostLog.h"
#include "lang/SclangProcess.h"
#include "params/ParameterBank.h"
#include "params/ParameterWatcher.h"

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

    static PluginGui& gui(const clap_plugin* plugin) { return *from(plugin)->mGui; }
    static ParameterBank& parameters(const clap_plugin* plugin) { return from(plugin)->mParameters; }

    const clap_plugin* clapPlugin() const { return &mClapPlugin; }

private:
    static Plugin* from(const clap_plugin* plugin);
    void post(const std::string& line);

    bool init();
    void destroy();
    bool activate(double sampleRate, uint32_t maxFrames);
    void deactivate();
    clap_process_status process(const clap_process* process);
    const void* extension(const char* id) const;
    void onTimer(clap_id timerId);
    void onMainThread();
    void applyParameterEvents();

    void startSclang();
    void run(const std::string& code);
    void evaluate(const std::string& code);
    void stopSound();
    void markProjectDirty();
    CodeController::Hooks codeHooks();
    EditorActions editorActions();
    bool saveState(const clap_ostream* stream) const;
    bool loadState(const clap_istream* stream);

    static uint32_t audioPortCount(const clap_plugin* plugin, bool isInput);
    static bool audioPortInfo(const clap_plugin* plugin, uint32_t index, bool isInput, clap_audio_port_info* info);
    static uint32_t latency(const clap_plugin* plugin);

    static const clap_plugin_audio_ports kAudioPorts;
    static const clap_plugin_latency kLatency;
    static const clap_plugin_timer_support kTimerSupport;
    static const clap_plugin_state kState;

    clap_plugin mClapPlugin;
    const clap_host* mHost;
    const clap_host_timer_support* mHostTimer = nullptr;
    const clap_host_posix_fd_support* mHostFd = nullptr;
    const clap_host_state* mHostState = nullptr;
    const clap_host_params* mHostParams = nullptr;
    clap_id mCodePollTimer = CLAP_INVALID_ID;
    PostLog mPostLog;
    ParameterBank mParameters;
    ParameterWatcher mWatcher;
    std::unique_ptr<OscPort> mOscPort;
    std::unique_ptr<SclangProcess> mSclang;
    std::unique_ptr<CodeController> mCode;
    std::unique_ptr<PluginGui> mGui;
    std::unique_ptr<Engine> mEngine;
    std::vector<float> mSilence;
};

}
