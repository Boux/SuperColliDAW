#include "Plugin.h"

#include "PluginPaths.h"
#include "code/DefaultCode.h"
#include "code/Examples.h"
#include "engine/InstalledSuperCollider.h"
#include "midi/MidiInput.h"
#include "params/ParameterEventReader.h"
#include "params/ParameterExtension.h"
#include "state/ClapStreams.h"

#include <cstdio>
#include <cstring>
#include <utility>

namespace supercollidaw {

namespace {

const char* const kFeatures[] = { CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_NOTE_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr };

constexpr clap_id kMainInputPortId = 0;
constexpr clap_id kMainOutputPortId = 1;
constexpr clap_id kNoteInputPortId = 0;
constexpr clap_id kNoteOutputPortId = 1;
constexpr uint32_t kCodePollIntervalMs = 250;

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

const clap_plugin_note_ports Plugin::kNotePorts = { .count = notePortCount, .get = notePortInfo };

const clap_plugin_latency Plugin::kLatency = { .get = latency };

const clap_plugin_timer_support Plugin::kTimerSupport = {
    .on_timer = [](const clap_plugin* plugin, clap_id timerId) { from(plugin)->onTimer(timerId); },
};

const clap_plugin_state Plugin::kState = {
    .save = [](const clap_plugin* plugin, const clap_ostream* stream) { return from(plugin)->saveState(stream); },
    .load = [](const clap_plugin* plugin, const clap_istream* stream) { return from(plugin)->loadState(stream); },
};

Plugin::Plugin(const clap_host* host):
    mHost(host),
    mWatcher([host] { host->request_callback(host); }),
    mCompletions(kCompletionsAddress, readCompletion, [host] { host->request_callback(host); }),
    mSignatures(kSignaturesAddress, readSignatureHelp, [host] { host->request_callback(host); }) {
    mClapPlugin.desc = &kDescriptor;
    mClapPlugin.plugin_data = this;
    mClapPlugin.init = [](const clap_plugin* plugin) { return from(plugin)->init(); };
    mClapPlugin.destroy = [](const clap_plugin* plugin) { from(plugin)->destroy(); };
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
    mClapPlugin.on_main_thread = [](const clap_plugin* plugin) { from(plugin)->onMainThread(); };
}

Plugin* Plugin::from(const clap_plugin* plugin) { return static_cast<Plugin*>(plugin->plugin_data); }

void Plugin::post(const std::string& line) {
    mPostLog.append(line);
    std::fprintf(stderr, "[SuperColliDAW] %s\n", line.c_str());
}

bool Plugin::init() {
    mHostTimer = static_cast<const clap_host_timer_support*>(mHost->get_extension(mHost, CLAP_EXT_TIMER_SUPPORT));
    if (mHostTimer && mHostTimer->register_timer)
        mHostTimer->register_timer(mHost, kCodePollIntervalMs, &mCodePollTimer);
    mHostFd = static_cast<const clap_host_posix_fd_support*>(mHost->get_extension(mHost, CLAP_EXT_POSIX_FD_SUPPORT));
    mHostState = static_cast<const clap_host_state*>(mHost->get_extension(mHost, CLAP_EXT_STATE));
    mHostParams = static_cast<const clap_host_params*>(mHost->get_extension(mHost, CLAP_EXT_PARAMS));
    mOscPort = std::make_unique<OscPort>([this](std::string_view packet) { return mCompletions.observe(packet) || mSignatures.observe(packet) || mWatcher.observe(packet); });
    mOutbox = std::make_unique<SclangOutbox>();
    mCode = std::make_unique<CodeController>(defaultCode(), codeHooks());
    mGui = std::make_unique<PluginGui>(mHost, mHostTimer, mHostFd, editorActions(), mPostLog, loadExamples(pluginResourcesDir() / "examples"));
    mGui->setCode(mCode->code());
    mGui->setStatus(mCode->status());
    startSclang();
    return true;
}

void Plugin::destroy() {
    if (mCodePollTimer != CLAP_INVALID_ID)
        mHostTimer->unregister_timer(mHost, mCodePollTimer);
    delete this;
}

void Plugin::startSclang() {
    const std::optional<std::string> executable = installedSclangPath();
    if (!executable) {
        post("sclang was not found. Install SuperCollider to run code.");
        return;
    }
    const std::string classLibraryDir = (pluginResourcesDir() / "classes").string();
    mSclang = std::make_unique<SclangProcess>(SclangProcess::Config{ *executable, classLibraryDir, mOscPort->port(), kNumChannels,
        kNumChannels, ParameterBank::kCount, [this](const std::string& line) { post(line); } });
    mOutbox->setSclangPort(mSclang->langPort());
}

void Plugin::stopSclang() {
    mOutbox->setSclangPort(0);
    mSclang.reset();
}

bool Plugin::activate(double sampleRate, uint32_t maxFrames) {
    auto engine = std::make_unique<Engine>(
        Engine::Config{ sampleRate, kNumChannels, kNumChannels, installedUGenPluginPath(), [this](const std::string& line) { post(line); } });
    if (!engine->isRunning())
        return false;
    mSilence.assign(maxFrames, 0.f);
    mEngine = std::move(engine);
    mTransport = std::make_unique<TransportFollower>(sampleRate);
    mTransportBuses = std::make_unique<TransportBuses>(sampleRate, ParameterBank::kCount);
    mOscPort->attach(mEngine.get());
    if (mSclang)
        mSclang->serverStarted();
    run(mCode->code());
    return true;
}

void Plugin::deactivate() {
    mOscPort->attach(nullptr);
    mEngine.reset();
    mTransport.reset();
    mTransportBuses.reset();
    mReleaseHeldNotes = true;
    if (mSclang)
        mSclang->serverStopped();
}

void Plugin::onTimer(clap_id timerId) {
    if (mGui->onTimer(timerId))
        return;
    if (timerId != mCodePollTimer)
        return;
    mCode->poll();
    applyParameterEvents();
    pollSclang();
}

void Plugin::onMainThread() {
    applyParameterEvents();
    showLanguageReplies();
}

void Plugin::applyParameterEvents() {
    const ParameterBank::Changes changes = mParameters.apply(mWatcher.takeEvents());
    const clap_param_rescan_flags flags = (changes.info ? CLAP_PARAM_RESCAN_INFO | CLAP_PARAM_RESCAN_TEXT : 0) | (changes.values ? CLAP_PARAM_RESCAN_VALUES : 0);
    if (flags && mHostParams && mHostParams->rescan)
        mHostParams->rescan(mHost, flags);
}

void Plugin::showLanguageReplies() {
    for (const Completion& completion : mCompletions.take())
        mGui->showCompletion(completion);
    for (const SignatureHelp& help : mSignatures.take())
        mGui->showSignatureHelp(help);
}

void Plugin::pollSclang() {
    const std::optional<int> exitStatus = mSclang ? mSclang->exitStatus() : std::nullopt;
    if (!exitStatus)
        return;
    post("sclang exited with code " + std::to_string(*exitStatus) + ". Reboot the interpreter to run code again.");
    stopSclang();
}

void Plugin::rebootInterpreter() {
    stopSclang();
    startSclang();
    if (mEngine)
        mHost->request_restart(mHost);
}

void Plugin::run(const std::string& code) {
    if (!mSclang || !mEngine)
        return;
    mWatcher.reset();
    mSclang->run(code);
}

void Plugin::evaluate(const std::string& code) {
    if (mSclang && mEngine)
        mSclang->evaluate(code);
}

void Plugin::complete(const std::string& line) {
    if (mSclang)
        mSclang->complete(line);
}

void Plugin::lookUpSignatures(const std::string& callee) {
    if (mSclang)
        mSclang->lookUpSignatures(callee);
}

void Plugin::stopSound() {
    if (mSclang)
        mSclang->stopSound();
}

void Plugin::markProjectDirty() {
    if (mHostState && mHostState->mark_dirty)
        mHostState->mark_dirty(mHost);
}

CodeController::Hooks Plugin::codeHooks() {
    return {
        .run = [this](const std::string& code) { run(code); },
        .post = [this](const std::string& line) { post(line); },
        .markProjectDirty = [this] { markProjectDirty(); },
        .showCode = [this](const std::string& code) { mGui->setCode(code); },
        .showStatus = [this](const EditorStatus& status) { mGui->setStatus(status); },
    };
}

EditorActions Plugin::editorActions() {
    return {
        .runAll = [this](const std::string& code) { mCode->runAll(code); },
        .evaluate = [this](const std::string& code) { evaluate(code); },
        .stop = [this] { stopSound(); },
        .rebootInterpreter = [this] { rebootInterpreter(); },
        .codeChanged = [this](const std::string& code) { mCode->edit(code); },
        .complete = [this](const std::string& line) { complete(line); },
        .lookUpSignatures = [this](const std::string& callee) { lookUpSignatures(callee); },
        .open = [this] { mCode->open(); },
        .save = [this](const std::string& code) { mCode->save(code); },
        .saveAs = [this](const std::string& code) { mCode->saveAs(code); },
        .unlink = [this](const std::string& code) { mCode->unlink(code); },
    };
}

bool Plugin::saveState(const clap_ostream* stream) const {
    PluginState state = mCode->state();
    state.parameters = mParameters.state();
    return writeAll(stream, encodeState(state));
}

bool Plugin::loadState(const clap_istream* stream) {
    const std::optional<std::string> bytes = readAll(stream);
    const std::optional<PluginState> state = bytes ? decodeState(*bytes) : std::nullopt;
    if (!state)
        return false;
    mParameters.restore(state->parameters);
    if (mHostParams && mHostParams->rescan)
        mHostParams->rescan(mHost, CLAP_PARAM_RESCAN_VALUES | CLAP_PARAM_RESCAN_INFO | CLAP_PARAM_RESCAN_TEXT);
    mCode->restore(*state);
    return true;
}

clap_process_status Plugin::process(const clap_process* process) {
    const float* inputs[kNumChannels];
    float* outputs[kNumChannels];
    for (uint32_t ch = 0; ch < kNumChannels; ++ch) {
        inputs[ch] = bufferChannel(process->audio_inputs, process->audio_inputs_count, ch, mSilence.data());
        outputs[ch] = bufferChannel(process->audio_outputs, process->audio_outputs_count, ch, nullptr);
        if (!outputs[ch])
            return CLAP_PROCESS_ERROR;
    }
    forwardMidi(process->in_events, *mOutbox);
    ParameterEventReader parameterEvents(mParameters, process->in_events);
    MidiOutput midiOutput(mHeldNotes, process->out_events);
    if (std::exchange(mReleaseHeldNotes, false))
        midiOutput.releaseHeldNotes(0);
    mTransportBuses->follow(process->transport, process->frames_count);
    mEngine->process(inputs, outputs, process->frames_count, { &parameterEvents, mTransportBuses.get() }, midiOutput);
    parameterEvents.applyAll();
    if (const auto transport = mTransport->follow(process->transport, mEngine->oscTimeAtFrame(0), process->frames_count))
        mOutbox->post(*transport);
    return CLAP_PROCESS_CONTINUE;
}

const void* Plugin::extension(const char* id) const {
    if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS))
        return &kAudioPorts;
    if (!std::strcmp(id, CLAP_EXT_NOTE_PORTS))
        return &kNotePorts;
    if (!std::strcmp(id, CLAP_EXT_LATENCY))
        return &kLatency;
    if (!std::strcmp(id, CLAP_EXT_TIMER_SUPPORT))
        return &kTimerSupport;
    if (!std::strcmp(id, CLAP_EXT_GUI))
        return &PluginGui::kExtension;
    if (!std::strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT))
        return &PluginGui::kPosixFdExtension;
    if (!std::strcmp(id, CLAP_EXT_STATE))
        return &kState;
    if (!std::strcmp(id, CLAP_EXT_PARAMS))
        return &kParameterExtension;
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

uint32_t Plugin::notePortCount(const clap_plugin*, bool) { return 1; }

bool Plugin::notePortInfo(const clap_plugin*, uint32_t index, bool isInput, clap_note_port_info* info) {
    if (index != 0)
        return false;
    info->id = isInput ? kNoteInputPortId : kNoteOutputPortId;
    info->supported_dialects = isInput ? CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_MIDI_MPE : CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    std::strncpy(info->name, isInput ? "MIDI In" : "MIDI Out", CLAP_NAME_SIZE);
    return true;
}

uint32_t Plugin::latency(const clap_plugin*) { return Engine::kLatency; }

}
