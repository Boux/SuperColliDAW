#include "Plugin.h"

#include "TestSynth.h"
#include "engine/InstalledSuperCollider.h"

#include <cstring>

namespace supercollidaw {

namespace {

const char* const kFeatures[] = { CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr };

constexpr clap_id kMainInputPortId = 0;
constexpr clap_id kMainOutputPortId = 1;

float* bufferChannel(const clap_audio_buffer* buffers, uint32_t count, uint32_t channel, float* fallback) {
    if (count == 0 || !buffers[0].data32 || channel >= buffers[0].channel_count)
        return fallback;
    return buffers[0].data32[channel];
}

}

const clap_plugin_descriptor Plugin::kDescriptor = {
    .clap_version = CLAP_VERSION_INIT,
    .id = "org.supercollidaw.supercollidaw",
    .name = "SuperColliDAW",
    .vendor = "SuperColliDAW",
    .url = "",
    .manual_url = "",
    .support_url = "",
    .version = "0.1.0",
    .description = "SuperCollider running inside the track",
    .features = kFeatures,
};

const clap_plugin_audio_ports Plugin::kAudioPorts = { .count = audioPortCount, .get = audioPortInfo };

const clap_plugin_latency Plugin::kLatency = { .get = latency };

Plugin::Plugin(const clap_host* host): mHost(host) {
    mClapPlugin.desc = &kDescriptor;
    mClapPlugin.plugin_data = this;
    mClapPlugin.init = [](const clap_plugin*) { return true; };
    mClapPlugin.destroy = [](const clap_plugin* plugin) { delete from(plugin); };
    mClapPlugin.activate = [](const clap_plugin* plugin, double sampleRate, uint32_t, uint32_t maxFrames) {
        return from(plugin)->activate(sampleRate, maxFrames);
    };
    mClapPlugin.deactivate = [](const clap_plugin* plugin) { from(plugin)->deactivate(); };
    mClapPlugin.start_processing = [](const clap_plugin*) { return true; };
    mClapPlugin.stop_processing = [](const clap_plugin*) {};
    mClapPlugin.reset = [](const clap_plugin*) {};
    mClapPlugin.process = [](const clap_plugin* plugin, const clap_process* process) {
        return from(plugin)->process(process);
    };
    mClapPlugin.get_extension = [](const clap_plugin* plugin, const char* id) { return from(plugin)->extension(id); };
    mClapPlugin.on_main_thread = [](const clap_plugin*) {};
}

Plugin* Plugin::from(const clap_plugin* plugin) { return static_cast<Plugin*>(plugin->plugin_data); }

bool Plugin::activate(double sampleRate, uint32_t maxFrames) {
    auto engine = std::make_unique<Engine>(Engine::Config{ sampleRate, kNumChannels, kNumChannels, installedUGenPluginPath() });
    if (!engine->isRunning())
        return false;
    startTestSynth(*engine);
    mSilence.assign(maxFrames, 0.f);
    mEngine = std::move(engine);
    return true;
}

void Plugin::deactivate() { mEngine.reset(); }

clap_process_status Plugin::process(const clap_process* process) {
    const float* inputs[kNumChannels];
    float* outputs[kNumChannels];
    for (uint32_t ch = 0; ch < kNumChannels; ++ch) {
        inputs[ch] = bufferChannel(process->audio_inputs, process->audio_inputs_count, ch, mSilence.data());
        outputs[ch] = bufferChannel(process->audio_outputs, process->audio_outputs_count, ch, nullptr);
        if (!outputs[ch])
            return CLAP_PROCESS_ERROR;
    }
    mEngine->process(inputs, outputs, process->frames_count);
    return CLAP_PROCESS_CONTINUE;
}

const void* Plugin::extension(const char* id) const {
    if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS))
        return &kAudioPorts;
    if (!std::strcmp(id, CLAP_EXT_LATENCY))
        return &kLatency;
    return nullptr;
}

uint32_t Plugin::audioPortCount(const clap_plugin*, bool) { return 1; }

bool Plugin::audioPortInfo(const clap_plugin*, uint32_t index, bool isInput, clap_audio_port_info* info) {
    if (index != 0)
        return false;
    info->id = isInput ? kMainInputPortId : kMainOutputPortId;
    std::strncpy(info->name, isInput ? "Main In" : "Main Out", CLAP_NAME_SIZE);
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = kNumChannels;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = isInput ? kMainOutputPortId : kMainInputPortId;
    return true;
}

uint32_t Plugin::latency(const clap_plugin*) { return Engine::kLatency; }

}
