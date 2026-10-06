#include "TestStreams.h"
#include "plugin/state/PluginState.h"

#include <clap/clap.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t kChannels = 2;
constexpr uint32_t kMaxFrames = 512;
constexpr uint32_t kLatency = 64;
constexpr double kTestSineHz = 440.0;
constexpr float kTestSineAmp = 0.1f;
constexpr clap_id kTimerId = 1;
constexpr auto kSclangStartTimeout = std::chrono::seconds(30);

std::filesystem::path gCodeFile;

std::string sineCode(double sineHz) {
    return "{ SoundIn.ar([0, 1]) + SinOsc.ar(" + std::to_string(sineHz) + ", 0, " + std::to_string(kTestSineAmp) + ") }.play;\n";
}

void writeCode(const std::filesystem::path& path, double sineHz) { std::ofstream(path) << sineCode(sineHz); }

void writeCode(double sineHz) { writeCode(gCodeFile, sineHz); }

const clap_host_timer_support kHostTimerSupport = {
    .register_timer = [](const clap_host*, uint32_t, clap_id* timerId) {
        *timerId = kTimerId;
        return true;
    },
    .unregister_timer = [](const clap_host*, clap_id) { return true; },
};

std::atomic<bool> gMainThreadCallbackRequested = false;
int gParameterRescans = 0;

const clap_host_params kHostParams = {
    .rescan = [](const clap_host*, clap_param_rescan_flags) { ++gParameterRescans; },
    .clear = [](const clap_host*, clap_id, clap_param_clear_flags) {},
    .request_flush = [](const clap_host*) {},
};

const clap_host kHost = {
    .clap_version = CLAP_VERSION_INIT,
    .host_data = nullptr,
    .name = "supercollidaw-test-host",
    .vendor = "",
    .url = "",
    .version = "0",
    .get_extension = [](const clap_host*, const char* id) -> const void* {
        if (std::string(id) == CLAP_EXT_PARAMS)
            return &kHostParams;
        return std::string(id) == CLAP_EXT_TIMER_SUPPORT ? &kHostTimerSupport : nullptr;
    },
    .request_restart = [](const clap_host*) {},
    .request_process = [](const clap_host*) {},
    .request_callback = [](const clap_host*) { gMainThreadCallbackRequested = true; },
};

struct InputEvents {
    std::vector<clap_event_param_value> values;
    std::vector<clap_event_param_mod> modulations;
    std::vector<clap_event_midi> midi;
    std::vector<const clap_event_header*> headers;

    const clap_input_events* list() {
        headers.clear();
        for (const auto& event : values)
            headers.push_back(&event.header);
        for (const auto& event : modulations)
            headers.push_back(&event.header);
        for (const auto& event : midi)
            headers.push_back(&event.header);
        static clap_input_events events;
        events = {
            .ctx = this,
            .size = [](const clap_input_events* e) { return static_cast<uint32_t>(static_cast<InputEvents*>(e->ctx)->headers.size()); },
            .get = [](const clap_input_events* e, uint32_t index) { return static_cast<InputEvents*>(e->ctx)->headers[index]; },
        };
        return &events;
    }

    void clear() {
        values.clear();
        modulations.clear();
        midi.clear();
    }
};

clap_event_header eventHeader(uint16_t type, uint32_t size) {
    return { .size = size, .time = 0, .space_id = CLAP_CORE_EVENT_SPACE_ID, .type = type, .flags = 0 };
}

struct HostTransport {
    bool playing = false;
    double tempo = 120.0;
    double songBeats = 0.0;

    clap_event_transport event() const {
        const double bar = std::floor(songBeats / 4.0);
        clap_event_transport transport{};
        transport.header = eventHeader(CLAP_EVENT_TRANSPORT, sizeof(clap_event_transport));
        transport.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE | CLAP_TRANSPORT_HAS_TIME_SIGNATURE | (playing ? CLAP_TRANSPORT_IS_PLAYING : 0);
        transport.song_pos_beats = std::llround(songBeats * CLAP_BEATTIME_FACTOR);
        transport.tempo = tempo;
        transport.bar_start = std::llround(bar * 4.0 * CLAP_BEATTIME_FACTOR);
        transport.bar_number = static_cast<int32_t>(bar);
        transport.tsig_num = 4;
        transport.tsig_denom = 4;
        return transport;
    }

    void advance(uint32_t frames, double sampleRate) { songBeats += playing ? frames / sampleRate * tempo / 60.0 : 0.0; }
};

struct OutputMidi {
    uint64_t frame;
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
};

struct OutputEvents {
    std::vector<OutputMidi> midi;
    uint64_t callbackStart = 0;
    uint32_t lastTime = 0;
    bool inOrder = true;

    const clap_output_events* list(uint64_t start) {
        callbackStart = start;
        lastTime = 0;
        static clap_output_events events;
        events = {
            .ctx = this,
            .try_push = [](const clap_output_events* e, const clap_event_header* header) {
                static_cast<OutputEvents*>(e->ctx)->push(*header);
                return true;
            },
        };
        return &events;
    }

    void push(const clap_event_header& header) {
        inOrder = inOrder && header.time >= lastTime;
        lastTime = header.time;
        if (header.space_id != CLAP_CORE_EVENT_SPACE_ID || header.type != CLAP_EVENT_MIDI)
            return;
        const auto& event = reinterpret_cast<const clap_event_midi&>(header);
        midi.push_back({ callbackStart + header.time, event.data[0], event.data[1], event.data[2] });
    }
};

int gFailures = 0;

void check(bool condition, const char* what) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", what);
    gFailures += condition ? 0 : 1;
}

double estimateFrequency(const std::vector<float>& signal, double sampleRate) {
    int crossings = 0;
    for (size_t i = 1; i < signal.size(); ++i)
        crossings += (signal[i - 1] < 0.f && signal[i] >= 0.f) ? 1 : 0;
    return crossings * sampleRate / signal.size();
}

class Instance {
public:
    Instance(const clap_plugin_factory* factory, double sampleRate): mSampleRate(sampleRate) {
        const clap_plugin_descriptor* descriptor = factory->get_plugin_descriptor(factory, 0);
        mPlugin = factory->create_plugin(factory, &kHost, descriptor->id);
        mActive = mPlugin && mPlugin->init(mPlugin) && mPlugin->activate(mPlugin, sampleRate, 1, kMaxFrames)
            && mPlugin->start_processing(mPlugin);
    }

    ~Instance() {
        if (!mPlugin)
            return;
        mPlugin->stop_processing(mPlugin);
        mPlugin->deactivate(mPlugin);
        mPlugin->destroy(mPlugin);
    }

    bool active() const { return mActive; }

    bool reactivate(double sampleRate) {
        mPlugin->stop_processing(mPlugin);
        mPlugin->deactivate(mPlugin);
        mSampleRate = sampleRate;
        mActive = mPlugin->activate(mPlugin, sampleRate, 1, kMaxFrames) && mPlugin->start_processing(mPlugin);
        return mActive;
    }
    const clap_plugin* clapPlugin() const { return mPlugin; }

    std::vector<float> run(const std::vector<float>& input, uint32_t framesPerCall) {
        std::vector<float> left(input.size()), right(input.size());
        for (size_t offset = 0; offset < input.size(); offset += framesPerCall) {
            const uint32_t frames = std::min<size_t>(framesPerCall, input.size() - offset);
            processCall(input.data() + offset, left.data() + offset, right.data() + offset, frames);
            pumpMainThread();
        }
        return left;
    }

    bool waitForSound(const std::function<void()>& beforeEachTry = [] {}) {
        const std::vector<float> silence(kMaxFrames, 0.f);
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::chrono::steady_clock::now() < deadline) {
            beforeEachTry();
            if (peak(run(silence, kMaxFrames)) > kTestSineAmp * 0.5f)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }

    bool waitForSignal(const std::function<bool(const std::vector<float>&)>& matches) {
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (matches(runSilence(0.05, kMaxFrames)))
                return true;
        }
        return false;
    }

    bool waitForPeakAbove(float level) {
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (peak(runSilence(0.05, kMaxFrames)) > level)
                return true;
        }
        return false;
    }

    bool waitForSilence() {
        const std::vector<float> silence(kMaxFrames, 0.f);
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (peak(run(silence, kMaxFrames)) < 1e-4f)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }

    std::vector<float> runRealtime(double seconds) { return runRealtime(std::vector<float>(static_cast<size_t>(seconds * mSampleRate), 0.f)); }

    std::vector<float> runRealtime(const std::vector<float>& input) {
        std::vector<float> left(input.size()), right(input.size());
        auto next = std::chrono::steady_clock::now();
        for (size_t offset = 0; offset < left.size(); offset += kMaxFrames) {
            const uint32_t frames = std::min<size_t>(kMaxFrames, left.size() - offset);
            processCall(input.data() + offset, left.data() + offset, right.data() + offset, frames);
            pumpMainThread();
            next += std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(frames / mSampleRate));
            std::this_thread::sleep_until(next);
        }
        return left;
    }

    void useTransport(HostTransport& transport) { mTransport = &transport; }

    void idle(double seconds) {
        std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
        pumpMainThread();
    }

    std::vector<float> runSilence(double seconds, uint32_t framesPerCall) {
        return run(std::vector<float>(static_cast<size_t>(seconds * mSampleRate), 0.f), framesPerCall);
    }

    std::string saveState() {
        std::string bytes;
        auto* state = static_cast<const clap_plugin_state*>(mPlugin->get_extension(mPlugin, CLAP_EXT_STATE));
        state->save(mPlugin, appendingStream(bytes));
        return bytes;
    }

    bool loadState(const std::string& bytes) {
        ReadCursor cursor{ bytes };
        auto* state = static_cast<const clap_plugin_state*>(mPlugin->get_extension(mPlugin, CLAP_EXT_STATE));
        return state->load(mPlugin, readingStream(cursor));
    }

    double waitForFrequency(double expectedHz, const std::function<void()>& beforeEachTry = [] {}) {
        double frequency = 0.0;
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::fabs(frequency - expectedHz) > 4.0 && std::chrono::steady_clock::now() < deadline) {
            beforeEachTry();
            frequency = estimateFrequency(runSilence(0.5, kMaxFrames), mSampleRate);
        }
        return frequency;
    }

    bool waitForMidiOut(uint8_t status, uint8_t data1) {
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::chrono::steady_clock::now() < deadline) {
            runSilence(0.05, kMaxFrames);
            if (std::ranges::any_of(mOutput.midi, [&](const OutputMidi& m) { return m.status == status && m.data1 == data1; }))
                return true;
        }
        return false;
    }

    std::vector<OutputMidi> takeMidiOut() { return std::exchange(mOutput.midi, {}); }

    bool midiOutInOrder() const { return mOutput.inOrder; }

    uint64_t framesProcessed() const { return mFrames; }

    void sendMidi(uint8_t status, uint8_t data1, uint8_t data2) {
        mEvents.midi.push_back({ .header = eventHeader(CLAP_EVENT_MIDI, sizeof(clap_event_midi)), .port_index = 0, .data = { status, data1, data2 } });
    }

    void setParameter(clap_id id, double value) {
        mEvents.values.push_back({ .header = eventHeader(CLAP_EVENT_PARAM_VALUE, sizeof(clap_event_param_value)), .param_id = id, .cookie = nullptr,
            .note_id = -1, .port_index = -1, .channel = -1, .key = -1, .value = value });
    }

    void modulateParameter(clap_id id, double amount) {
        mEvents.modulations.push_back({ .header = eventHeader(CLAP_EVENT_PARAM_MOD, sizeof(clap_event_param_mod)), .param_id = id, .cookie = nullptr,
            .note_id = -1, .port_index = -1, .channel = -1, .key = -1, .amount = amount });
    }

    const clap_plugin_params* params() const {
        return static_cast<const clap_plugin_params*>(mPlugin->get_extension(mPlugin, CLAP_EXT_PARAMS));
    }

    clap_param_info parameterInfo(uint32_t index) const {
        clap_param_info info{};
        params()->get_info(mPlugin, index, &info);
        return info;
    }

    bool waitForParameter(uint32_t index, const char* name) {
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::chrono::steady_clock::now() < deadline) {
            const clap_param_info info = parameterInfo(index);
            if (!(info.flags & CLAP_PARAM_IS_HIDDEN) && std::string(info.name) == name)
                return true;
            runSilence(0.05, kMaxFrames);
        }
        return false;
    }

    void fireTimer() {
        auto* timer = static_cast<const clap_plugin_timer_support*>(mPlugin->get_extension(mPlugin, CLAP_EXT_TIMER_SUPPORT));
        timer->on_timer(mPlugin, kTimerId);
    }

    static float peak(const std::vector<float>& signal) {
        float result = 0.f;
        for (float sample : signal)
            result = std::max(result, std::fabs(sample));
        return result;
    }

    double sampleRate() const { return mSampleRate; }

private:
    void pumpMainThread() {
        if (gMainThreadCallbackRequested.exchange(false))
            mPlugin->on_main_thread(mPlugin);
    }

    void processCall(const float* in, float* outLeft, float* outRight, uint32_t frames) {
        float* inChannels[kChannels] = { const_cast<float*>(in), const_cast<float*>(in) };
        float* outChannels[kChannels] = { outLeft, outRight };
        clap_audio_buffer input = { .data32 = inChannels, .data64 = nullptr, .channel_count = kChannels, .latency = 0, .constant_mask = 0 };
        clap_audio_buffer output = { .data32 = outChannels, .data64 = nullptr, .channel_count = kChannels, .latency = 0, .constant_mask = 0 };
        const clap_event_transport transport = mTransport ? mTransport->event() : clap_event_transport{};
        clap_process process = {
            .steady_time = -1,
            .frames_count = frames,
            .transport = mTransport ? &transport : nullptr,
            .audio_inputs = &input,
            .audio_outputs = &output,
            .audio_inputs_count = 1,
            .audio_outputs_count = 1,
            .in_events = mEvents.list(),
            .out_events = mOutput.list(mFrames),
        };
        mPlugin->process(mPlugin, &process);
        mEvents.clear();
        mFrames += frames;
        if (mTransport)
            mTransport->advance(frames, mSampleRate);
    }

    double mSampleRate;
    InputEvents mEvents;
    OutputEvents mOutput;
    uint64_t mFrames = 0;
    HostTransport* mTransport = nullptr;
    const clap_plugin* mPlugin = nullptr;
    bool mActive = false;
};

void testSineFollowsHostSampleRate(const clap_plugin_factory* factory, double sampleRate) {
    Instance instance(factory, sampleRate);
    check(instance.active(), "plugin activates");
    check(instance.waitForSound(), "sclang runs the code file and it makes sound");

    const std::vector<float> out = instance.runSilence(1.0, 100);
    const double frequency = estimateFrequency(out, sampleRate);
    std::printf("  sample rate %.0f: sine peak %.4f, frequency %.2f Hz\n", sampleRate, Instance::peak(out), frequency);
    check(std::fabs(Instance::peak(out) - kTestSineAmp) < 0.01f, "sine amplitude is 0.1");
    check(std::fabs(frequency - kTestSineHz) < 2.0, "sine is 440 Hz at the host sample rate");
}

void testTrackInputPassesThroughWithReportedLatency(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();

    constexpr size_t kImpulseAt = 1000;
    std::vector<float> input(4800, 0.f);
    input[kImpulseAt] = 1.f;
    const std::vector<float> out = instance.run(input, 37);

    size_t loudest = 0;
    for (size_t i = 0; i < out.size(); ++i)
        loudest = std::fabs(out[i]) > std::fabs(out[loudest]) ? i : loudest;
    std::printf("  impulse in at %zu, loudest out at %zu (value %.3f)\n", kImpulseAt, loudest, out[loudest]);
    check(loudest == kImpulseAt + kLatency, "track input reaches the output after exactly the reported latency");
}

void testTwoInstancesRunTogether(const clap_plugin_factory* factory) {
    Instance first(factory, 48000.0);
    Instance second(factory, 48000.0);
    check(first.active() && second.active(), "two instances activate at the same time");
    check(first.waitForSound() && second.waitForSound(), "both instances produce sound");
}

void testStateRestoresCode(const clap_plugin_factory* factory) {
    std::string saved;
    {
        Instance original(factory, 48000.0);
        original.waitForSound();
        saved = original.saveState();
    }
    writeCode(880.0);
    Instance restored(factory, 48000.0);
    restored.waitForSound();
    check(restored.loadState(saved), "the saved state loads into a new instance");
    const double frequency = restored.waitForFrequency(kTestSineHz);
    std::printf("  restored instance: frequency %.2f Hz\n", frequency);
    check(std::fabs(frequency - kTestSineHz) <= 4.0, "a loaded state replaces the template code and runs it");
    writeCode(kTestSineHz);
}

void testFileIsNeverLoadedOnItsOwn(const clap_plugin_factory* factory) {
    const std::filesystem::path file = gCodeFile.parent_path() / "opened.scd";
    writeCode(file, 880.0);
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    check(instance.loadState(supercollidaw::encodeState({ sineCode(660.0), file.string() })), "a state with a file path loads");
    const double restored = instance.waitForFrequency(660.0);
    std::printf("  restored while the file plays 880 Hz: frequency %.2f Hz\n", restored);
    check(std::fabs(restored - 660.0) <= 4.0, "a loaded state runs its own code and leaves the file alone");
    writeCode(file, 440.0);
    instance.fireTimer();
    instance.idle(1.0);
    const double frequency = estimateFrequency(instance.runSilence(0.5, kMaxFrames), 48000.0);
    std::printf("  after another instance saves the file: frequency %.2f Hz\n", frequency);
    check(std::fabs(frequency - 660.0) <= 4.0, "saving the file elsewhere never changes this instance");
}

void testReactivationKeepsServerNotifications(const clap_plugin_factory* factory) {
    const std::string code = "OSCFunc({ { SinOsc.ar(660, 0, 0.1) }.play }, '/tr').oneShot;\n{ SendTrig.kr(Impulse.kr(10)); Silent.ar }.play;\n";
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    const double before = instance.waitForFrequency(660.0);
    std::printf("  before reactivating: %.2f Hz\n", before);
    check(std::fabs(before - 660.0) <= 4.0, "a SendTrig reply reaches sclang and starts a synth");
    check(instance.reactivate(44100.0), "the plugin reactivates at another sample rate");
    const double after = instance.waitForFrequency(660.0);
    std::printf("  after reactivating: %.2f Hz\n", after);
    check(std::fabs(after - 660.0) <= 4.0, "server notifications still reach sclang after the host reactivates the plugin");
}

void testStuckSclangDoesNotBlockTheHost(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "{ SinOsc.ar(660, 0, 0.1) }.play;\ninf.do {};\n", "", {} }));
    instance.waitForFrequency(660.0);
    const std::string moreThanAPipeHolds = "// " + std::string(256 * 1024, 'x') + "\n";
    const auto start = std::chrono::steady_clock::now();
    for (int run = 0; run < 4; ++run)
        instance.loadState(supercollidaw::encodeState({ moreThanAPipeHolds, "", {} }));
    const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
    std::printf("  sending 1 MB of code to a stuck sclang took %.3f s\n", elapsed.count());
    check(elapsed.count() < 1.0, "a stuck sclang does not block the host's main thread");
}

void testTrackMidiReachesMIDIdef(const clap_plugin_factory* factory) {
    const std::string code = "MIDIClient.init;\n"
                             "MIDIIn.connectAll;\n"
                             "if(MIDIClient.sources.collect(_.device) != [\"SuperColliDAW\"]) { Error(\"MIDIClient lists more than the track\").throw };\n"
                             "MIDIdef.noteOn(\\test, { |vel, note| ~synth ?? { ~synth = { SinOsc.ar(note.midicps, 0, vel / 1270) }.play } });\n";
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    const double frequency = instance.waitForFrequency(880.0, [&] { instance.sendMidi(0x90, 81, 127); });
    std::printf("  note 81 from the track: %.2f Hz\n", frequency);
    check(std::fabs(frequency - 880.0) <= 4.0, "a note from the track reaches MIDIdef.noteOn, and MIDIClient lists only the track");
}

void testInstrumentPlaysTrackNotes(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "SuperColliDAW.instrument { |freq, amp| SinOsc.ar(freq, 0, amp * 0.1) };\n", "", {} }));
    instance.waitForSilence();
    instance.waitForSound([&] { instance.sendMidi(0x90, 81, 127); });
    const double frequency = instance.waitForFrequency(880.0);
    std::printf("  note 81 on: %.2f Hz\n", frequency);
    check(std::fabs(frequency - 880.0) <= 4.0, "SuperColliDAW.instrument plays a synth for a note from the track");
    instance.sendMidi(0x80, 81, 0);
    check(instance.waitForSilence(), "a note-off from the track releases that synth");
}

void testInstrumentPassesNoteDetails(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    const std::string code = "SuperColliDAW.instrument { |midinote, velocity, chan| SinOsc.ar((midinote + chan).midicps, 0, velocity / 1270) };\n";
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    instance.waitForSilence();
    instance.waitForSound([&] { instance.sendMidi(0x91, 80, 127); });
    const double frequency = instance.waitForFrequency(880.0);
    std::printf("  note 80 on channel 1: %.2f Hz\n", frequency);
    check(std::fabs(frequency - 880.0) <= 4.0, "the instrument passes midinote, velocity and chan to each synth");
}

void testInstrumentBendsPitch(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "SuperColliDAW.instrument { |freq, amp| SinOsc.ar(freq, 0, amp * 0.1) };\n", "", {} }));
    instance.waitForSilence();
    instance.waitForSound([&] { instance.sendMidi(0x90, 69, 127); });
    instance.sendMidi(0xE0, 0x7F, 0x7F);
    const double bent = instance.waitForFrequency(493.88);
    instance.sendMidi(0x80, 69, 0);
    instance.waitForSilence();
    instance.sendMidi(0x90, 57, 127);
    const double started = instance.waitForFrequency(246.94);
    std::printf("  note 69 bent fully up: %.2f Hz, note 57 started while bent: %.2f Hz\n", bent, started);
    check(std::fabs(bent - 493.88) <= 4.0, "pitch bend reaches the playing voice as bend, two semitones by default");
    check(std::fabs(started - 246.94) <= 4.0, "a note started while the wheel is bent starts bent");

    instance.loadState(supercollidaw::encodeState({ "SuperColliDAW.instrument { |noteFreq, bend| SinOsc.ar(noteFreq, 0, 0.1 * (bend > 1)) };\n", "", {} }));
    instance.waitForSilence();
    const double unbent = instance.waitForFrequency(440.0, [&] {
        instance.sendMidi(0x90, 69, 127);
        instance.sendMidi(0xE0, 0x7F, 0x7F);
    });
    std::printf("  noteFreq of note 69 with the wheel fully up: %.2f Hz\n", unbent);
    check(std::fabs(unbent - 440.0) <= 4.0, "noteFreq stays the key's pitch while freq follows the bend");
}

void testInstrumentSustainPedal(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "SuperColliDAW.instrument { |freq, amp| SinOsc.ar(freq, 0, amp * 0.1) };\n", "", {} }));
    instance.waitForSilence();
    instance.waitForSound([&] { instance.sendMidi(0x90, 69, 127); });
    instance.sendMidi(0xB0, 64, 127);
    instance.runSilence(0.2, kMaxFrames);
    instance.sendMidi(0x80, 69, 0);
    instance.runSilence(1.0, kMaxFrames);
    const float sustained = Instance::peak(instance.runSilence(0.2, kMaxFrames));
    instance.sendMidi(0xB0, 64, 0);
    const bool released = instance.waitForSilence();
    std::printf("  1 s after the note-off with the pedal down: peak %.3f\n", sustained);
    check(sustained > kTestSineAmp * 0.5f, "the sustain pedal holds a released note");
    check(released, "lifting the pedal releases it");
}

void testInstrumentFollowsMpeBend(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "SuperColliDAW.instrument { |freq| SinOsc.ar(freq, 0, 0.1) };\n", "", {} }));
    instance.waitForSilence();
    instance.waitForSound([&] { instance.sendMidi(0x91, 57, 127); });
    instance.sendMidi(0xE1, 0, 96);
    const double perNote = instance.waitForFrequency(880.0);
    instance.sendMidi(0xE0, 127, 127);
    const double withMaster = instance.waitForFrequency(987.76);
    instance.sendMidi(0xB1, 101, 0);
    instance.sendMidi(0xB1, 100, 0);
    instance.sendMidi(0xB1, 6, 12);
    const double withRange = instance.waitForFrequency(349.2);
    std::printf("  note 57 on channel 2 bent halfway: %.2f Hz, plus channel 1 fully up: %.2f Hz, after RPN 0 sets channel 2 to 12: %.2f Hz\n", perNote, withMaster, withRange);
    check(std::fabs(perNote - 880.0) <= 4.0, "per-note bend on channels 2 to 16 spans MPE's 48 semitones");
    check(std::fabs(withMaster - 987.76) <= 4.0, "bend on channel 1, the MPE master channel, adds to every voice");
    check(std::fabs(withRange - 349.2) <= 4.0, "Pitch Bend Sensitivity (RPN 0) sets a channel's bend range");
}

void testInstrumentFollowsMpeTimbreAndPressure(const clap_plugin_factory* factory) {
    const std::string code = "SuperColliDAW.instrument { |freq, timbre, pressure| SinOsc.ar(freq * (1 + timbre), 0, 0.02 + (pressure * 0.08)) };\n";
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    instance.waitForSilence();
    const double resting = instance.waitForFrequency(220.0, [&] { instance.sendMidi(0x92, 57, 127); });
    instance.sendMidi(0xB2, 74, 127);
    const double bright = instance.waitForFrequency(440.0);
    instance.sendMidi(0xD2, 127, 0);
    const bool pressed = instance.waitForPeakAbove(0.08f);
    instance.sendMidi(0xD2, 0, 0);
    instance.waitForSignal([](const std::vector<float>& signal) { return Instance::peak(signal) < 0.05f; });
    instance.sendMidi(0xD0, 127, 0);
    const bool pressedOnMaster = instance.waitForPeakAbove(0.08f);
    std::printf("  note 57 on channel 3: %.2f Hz at rest, %.2f Hz with CC 74 at 127\n", resting, bright);
    check(std::fabs(resting - 220.0) <= 4.0, "a voice starts with timbre 0 until CC 74 arrives");
    check(std::fabs(bright - 440.0) <= 4.0, "CC 74 at 127 on a note's channel sets that voice's timbre to 1");
    check(pressed, "channel pressure on a note's channel sets that voice's pressure");
    check(pressedOnMaster, "pressure on channel 1 reaches voices on the other channels");
}

void testInstrumentMode(const clap_plugin_factory* factory, const std::string& mode, bool glides) {
    const std::string code = "~ready = { SinOsc.ar(660, 0, 0.1) }.play;\n"
                             "MIDIdef.cc(\\ready, { ~ready.release }, 1);\n"
                             "SuperColliDAW.instrument { |freq, amp| SinOsc.ar(freq, 0, amp * 0.1 * Line.kr(0, 1, 4)) }.mode_(" + mode + ");\n";
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    instance.waitForFrequency(660.0);
    instance.sendMidi(0xB0, 1, 127);
    instance.waitForSilence();
    instance.sendMidi(0x90, 69, 127);
    instance.waitForFrequency(440.0);
    instance.runSilence(4.5, kMaxFrames);
    instance.sendMidi(0x90, 81, 127);
    const double newest = instance.waitForFrequency(880.0);
    const float newestPeak = Instance::peak(instance.runSilence(0.2, kMaxFrames));
    instance.sendMidi(0x80, 81, 0);
    const double back = instance.waitForFrequency(440.0);
    instance.sendMidi(0x80, 69, 0);
    const bool silent = instance.waitForSilence();
    std::printf("  %s: newest key %.2f Hz at peak %.3f, %.2f Hz after releasing it\n", mode.c_str(), newest, newestPeak, back);
    check(std::fabs(newest - 880.0) <= 4.0, "a mono instrument plays only the newest held key");
    check(glides ? newestPeak > 0.08f : newestPeak < 0.05f, glides ? "legato changes the pitch of the playing synth" : "mono restarts the synth for each key");
    check(std::fabs(back - 440.0) <= 4.0, "releasing the newest key goes back to the key still held");
    check(silent, "releasing the last key releases the voice");
}

std::vector<size_t> clickFrames(const std::vector<float>& signal) {
    std::vector<size_t> frames;
    for (size_t frame = 1; frame < signal.size(); ++frame) {
        if (signal[frame] > 0.25f && signal[frame - 1] <= 0.25f)
            frames.push_back(frame);
    }
    return frames;
}

std::vector<size_t> beatFrames(double startBeats, double tempo, int firstBeat, int lastBeat, double sampleRate) {
    std::vector<size_t> frames;
    for (int beat = firstBeat; beat <= lastBeat; ++beat)
        frames.push_back(static_cast<size_t>(std::llround((beat - startBeats) * 60.0 / tempo * sampleRate)) + kLatency);
    return frames;
}

double worstOffsetMs(const std::vector<size_t>& clicks, const std::vector<size_t>& beats, double sampleRate) {
    double worst = 0.0;
    for (size_t beat : beats) {
        const auto nearest = std::min_element(clicks.begin(), clicks.end(), [beat](size_t a, size_t b) {
            return std::llabs(static_cast<long long>(a) - static_cast<long long>(beat)) < std::llabs(static_cast<long long>(b) - static_cast<long long>(beat));
        });
        const double offset = nearest == clicks.end() ? 1e9 : std::llabs(static_cast<long long>(*nearest) - static_cast<long long>(beat));
        worst = std::max(worst, offset / sampleRate * 1000.0);
    }
    return worst;
}

void testClockFollowsTransport(const clap_plugin_factory* factory) {
    const std::string code = "SynthDef(\\click, { |out| OffsetOut.ar(out, Impulse.ar(0) * 0.5); Line.kr(0, 0, 0.01, doneAction: 2) }).add;\n"
                             "~ready = { SinOsc.ar(660, 0, 0.1) }.play;\n"
                             "SuperColliDAW.onPlay { ~ready.release; ~player = Pbind(\\instrument, \\click, \\dur, 1).play(quant: 4) };\n"
                             "SuperColliDAW.onStop { ~player.stop; { SinOsc.ar(990, 0, 0.1) }.play };\n";
    HostTransport transport;
    transport.songBeats = 2.5;
    Instance instance(factory, 48000.0);
    instance.useTransport(transport);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    instance.waitForFrequency(660.0);

    transport.playing = true;
    const std::vector<size_t> at120 = clickFrames(instance.runRealtime(4.0));
    transport.tempo = 150.0;
    const std::vector<size_t> at150 = clickFrames(instance.runRealtime(3.0));
    transport.songBeats -= 8.0;
    const std::vector<size_t> afterLoop = clickFrames(instance.runRealtime(2.0));

    const std::vector<size_t> beatsAt120 = beatFrames(2.5, 120.0, 4, 10, 48000.0);
    const double offset120 = worstOffsetMs(at120, beatsAt120, 48000.0);
    std::printf("  started at beat 2.5 with quant 4: first click at frame %zu, beat 4 at %zu, worst offset %.2f ms\n", at120.empty() ? 0 : at120.front(),
        beatsAt120.front(), offset120);
    check(!at120.empty() && at120.front() + 48 > beatsAt120.front(), "a pattern started on play waits for the next DAW bar");
    check(offset120 < 1.0, "pattern events land on the DAW's beats at 120 bpm");

    const double offset150 = worstOffsetMs(at150, beatFrames(10.5, 150.0, 12, 17, 48000.0), 48000.0);
    std::printf("  after the DAW switches to 150 bpm: worst offset %.2f ms\n", offset150);
    check(offset150 < 1.0, "pattern events follow a tempo change in the DAW");

    const double offsetAfterLoop = worstOffsetMs(afterLoop, beatFrames(10.0, 150.0, 11, 14, 48000.0), 48000.0);
    std::printf("  after the DAW loops back 2 bars: worst offset %.2f ms\n", offsetAfterLoop);
    check(offsetAfterLoop < 1.0, "pattern events keep landing on the DAW's beats when it loops back");

    transport.playing = false;
    const double stoppedFrequency = instance.waitForFrequency(990.0);
    std::printf("  after the DAW stops: %.2f Hz\n", stoppedFrequency);
    check(std::fabs(stoppedFrequency - 990.0) <= 4.0, "SuperColliDAW.onStop runs when the DAW stops");
}

std::vector<OutputMidi> midiWithStatus(const std::vector<OutputMidi>& midi, uint8_t status) {
    std::vector<OutputMidi> matching;
    std::ranges::copy_if(midi, std::back_inserter(matching), [status](const OutputMidi& m) { return m.status == status; });
    return matching;
}

void testPatternMidiReachesTheTrack(const clap_plugin_factory* factory) {
    const std::string code = "SynthDef(\\click, { |out| OffsetOut.ar(out, Impulse.ar(0) * 0.5); Line.kr(0, 0, 0.01, doneAction: 2) }).add;\n"
                             "MIDIClient.init;\n"
                             "~out = MIDIOut.newByName(\"SuperColliDAW\", \"Track\");\n"
                             "Ppar([Pbind(\\instrument, \\click, \\dur, 0.5), Pbind(\\type, \\midi, \\midiout, ~out, \\midinote, 60, \\dur, 0.5, \\legato, 0.5)]).play;\n";
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    check(instance.waitForMidiOut(0x90, 60), "a Pbind with \\type \\midi sends notes to the track through MIDIOut");

    instance.takeMidiOut();
    const uint64_t start = instance.framesProcessed();
    const std::vector<size_t> clicks = clickFrames(instance.runRealtime(3.0));
    const uint64_t end = instance.framesProcessed();
    instance.runRealtime(0.5);
    const std::vector<OutputMidi> midi = instance.takeMidiOut();
    std::vector<OutputMidi> noteOns = midiWithStatus(midi, 0x90);
    std::erase_if(noteOns, [end](const OutputMidi& m) { return m.frame >= end; });
    const std::vector<OutputMidi> noteOffs = midiWithStatus(midi, 0x80);

    std::vector<size_t> noteOnFrames;
    std::ranges::transform(noteOns, std::back_inserter(noteOnFrames), [start](const OutputMidi& m) { return static_cast<size_t>(m.frame - start); });
    const double offset = worstOffsetMs(clicks, noteOnFrames, 48000.0);
    std::printf("  %zu note-ons and %zu clicks in 3 s, worst offset between a note-on and its click %.3f ms\n", noteOns.size(), clicks.size(), offset);
    check(noteOns.size() >= 5, "the pattern keeps sending notes");
    check(offset <= 1.0 / 48.0, "each note-on lands on the same sample as the click from the same event");

    const bool released = std::ranges::all_of(noteOns, [&](const OutputMidi& on) {
        return std::ranges::any_of(noteOffs, [&](const OutputMidi& off) { return off.data1 == on.data1 && std::llabs(static_cast<long long>(off.frame - on.frame) - 12000) <= 1; });
    });
    check(released, "each note-off follows its note-on by the event's sustain");
    check(instance.midiOutInOrder(), "MIDI events go to the host in sample order");
}

void testRunAllReleasesHeldMidiNotes(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "MIDIOut(0).noteOn(0, 64, 100);\n", "", {} }));
    instance.waitForMidiOut(0x90, 64);
    instance.takeMidiOut();
    instance.loadState(supercollidaw::encodeState({ "", "", {} }));
    check(instance.waitForMidiOut(0x80, 64), "Run all releases a note that MIDIOut left on");
    const std::vector<OutputMidi> midi = instance.takeMidiOut();
    const auto allNotesOff = std::ranges::count_if(midi, [](const OutputMidi& m) { return (m.status & 0xF0) == 0xB0 && m.data1 == 123; });
    std::printf("  after Run all: %zu MIDI events, %td All Notes Off\n", midi.size(), allNotesOff);
    check(allNotesOff == 16, "Run all sends All Notes Off on every channel");
}

void testReactivationReleasesHeldMidiNotes(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "MIDIOut(0).noteOn(2, 65, 100);\n", "", {} }));
    instance.waitForMidiOut(0x92, 65);
    instance.takeMidiOut();
    instance.reactivate(48000.0);
    const uint64_t start = instance.framesProcessed();
    instance.runSilence(0.001, kMaxFrames);
    const std::vector<OutputMidi> midi = instance.takeMidiOut();
    const bool released = !midi.empty() && midi.front().status == 0x82 && midi.front().data1 == 65 && midi.front().frame == start;
    check(released, "the first process after a reactivation releases notes the old server left on");
}

size_t nonFiniteSamples(const std::vector<float>& signal) { return std::ranges::count_if(signal, [](float sample) { return !std::isfinite(sample); }); }

void testBrokenSynthStaysInsideThePlugin(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "{ SinOsc.ar(220) * 100 }.play;\n", "", {} }));
    instance.waitForPeakAbove(1.0f);
    const float loudPeak = Instance::peak(instance.runSilence(0.5, kMaxFrames));
    std::printf("  sine at amplitude 100: peak %.3f\n", loudPeak);
    check(loudPeak > 1.2f && loudPeak <= 1.26f, "output is clipped at the server's safety clip threshold, 1.26 by default");

    instance.loadState(supercollidaw::encodeState({ "{ RLPF.ar(Saw.ar(110), In.kr(0).linexp(0, 1, 0, 4000)) * 0.1 + SinOsc.ar(660, 0, 0.1) }.play;\n", "", {} }));
    instance.waitForFrequency(660.0);
    instance.setParameter(0, 0.5);
    instance.runSilence(0.2, kMaxFrames);
    const std::vector<float> broken = instance.runSilence(0.5, kMaxFrames);
    std::printf("  filter with a NaN cutoff: %zu non-finite samples\n", nonFiniteSamples(broken));
    check(nonFiniteSamples(broken) == 0, "a synth that outputs NaN does not send NaN to the host");

    instance.loadState(supercollidaw::encodeState({ sineCode(kTestSineHz), "", {} }));
    const double frequency = instance.waitForFrequency(kTestSineHz);
    std::printf("  after Run all with a plain sine: %.2f Hz\n", frequency);
    check(std::fabs(frequency - kTestSineHz) <= 4.0, "Run all brings the sound back after a synth blew up");
}

class StderrCapture {
public:
    explicit StderrCapture(const std::filesystem::path& path): mSaved(dup(STDERR_FILENO)) {
        std::fflush(stderr);
        const int file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(file, STDERR_FILENO);
        close(file);
    }

    ~StderrCapture() {
        std::fflush(stderr);
        dup2(mSaved, STDERR_FILENO);
        close(mSaved);
    }

private:
    int mSaved;
};

std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

size_t firstError(const std::string& log) { return std::min(log.find("ERROR"), log.find("FAILURE IN SERVER")); }

bool waitForSoundOrNotes(Instance& instance) {
    std::vector<float> input(kMaxFrames * 10);
    for (size_t frame = 0; frame < input.size(); ++frame)
        input[frame] = 0.1f * static_cast<float>(std::sin(2.0 * M_PI * 330.0 * frame / instance.sampleRate()));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (std::chrono::steady_clock::now() < deadline) {
        instance.sendMidi(0x90, 60, 100);
        const float level = Instance::peak(instance.runRealtime(input));
        const std::vector<OutputMidi> midi = instance.takeMidiOut();
        if (level > 0.01f || std::ranges::any_of(midi, [](const OutputMidi& m) { return (m.status & 0xF0) == 0x90; }))
            return true;
    }
    return false;
}

bool hasCode(const std::string& text) {
    std::istringstream lines(text);
    for (std::string line; std::getline(lines, line);) {
        const size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos && line.compare(start, 2, "//") != 0)
            return true;
    }
    return false;
}

void testExampleRuns(const clap_plugin_factory* factory, const std::filesystem::path& example) {
    const std::filesystem::path logPath = gCodeFile.parent_path() / "example.log";
    bool active = false;
    {
        StderrCapture capture(logPath);
        HostTransport transport;
        transport.playing = true;
        Instance instance(factory, 48000.0);
        instance.useTransport(transport);
        instance.loadState(supercollidaw::encodeState({ readFile(example), "", {} }));
        active = waitForSoundOrNotes(instance);
    }
    const std::string log = readFile(logPath);
    const bool clean = firstError(log) == std::string::npos;
    const bool playable = hasCode(readFile(example));
    std::printf("  %s: %s, %s\n", example.filename().c_str(), active ? "sound or notes" : playable ? "nothing" : "no code to play", clean ? "no errors" : "errors in the post window:");
    if (!clean)
        std::printf("%s\n", log.substr(firstError(log), 1200).c_str());
    check((active || !playable) && clean, ("example " + example.filename().string() + " runs without errors and makes sound or notes").c_str());
}

void testExamplesRun(const clap_plugin_factory* factory, const std::filesystem::path& examplesDir) {
    std::vector<std::filesystem::path> examples;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(examplesDir))
        examples.push_back(entry.path());
    std::ranges::sort(examples);
    check(!examples.empty(), "examples are installed next to the plugin");
    std::ofstream(gCodeFile) << "// silent\n";
    for (const std::filesystem::path& example : examples)
        testExampleRuns(factory, example);
    writeCode(kTestSineHz);
}

size_t occurrences(const std::string& text, const std::string& pattern) {
    size_t count = 0;
    for (size_t at = text.find(pattern); at != std::string::npos; at = text.find(pattern, at + 1))
        ++count;
    return count;
}

void testUnprocessedInstanceRecovers(const clap_plugin_factory* factory) {
    const std::filesystem::path logPath = gCodeFile.parent_path() / "unprocessed.log";
    double frequency = 0.0;
    {
        StderrCapture capture(logPath);
        Instance instance(factory, 48000.0);
        instance.idle(20.0);
        frequency = instance.waitForFrequency(kTestSineHz);
    }
    const size_t refused = occurrences(readFile(logPath), "/notify : already registered");
    std::printf("  after 20 s activated but not processed: %zu refused /notify requests, then %.2f Hz\n", refused, frequency);
    check(refused == 0, "sclang does not flood the server when the host resumes processing an instance it left activated");
    check(std::fabs(frequency - kTestSineHz) <= 4.0, "the code runs once the host resumes processing");
}

// The output carries SuperColliDAW.beats / 1000 as DC, and output frame n plays the input frame n - kLatency.
double worstBeatsError(const std::vector<float>& signal, double startBeats, double tempo, double sampleRate) {
    double worst = 0.0;
    for (size_t frame = 3 * kLatency; frame < signal.size(); ++frame) {
        const double expected = startBeats + (static_cast<double>(frame) - kLatency) / sampleRate * tempo / 60.0;
        worst = std::max(worst, std::fabs(signal[frame] * 1000.0 - expected));
    }
    return worst;
}

void testTransportUGens(const clap_plugin_factory* factory) {
    HostTransport transport;
    transport.playing = true;
    transport.songBeats = 100.0;
    Instance instance(factory, 48000.0);
    instance.useTransport(transport);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ "{ SinOsc.ar(SuperColliDAW.bpm * 4, 0, 0.1) }.play;\n", "", {} }));
    const double at120 = instance.waitForFrequency(480.0);
    transport.tempo = 90.0;
    const double at90 = instance.waitForFrequency(360.0);
    std::printf("  SinOsc.ar(SuperColliDAW.bpm * 4): %.2f Hz at 120 bpm, %.2f Hz at 90 bpm\n", at120, at90);
    check(std::fabs(at120 - 480.0) <= 4.0 && std::fabs(at90 - 360.0) <= 4.0, "SuperColliDAW.bpm follows the DAW's tempo");

    instance.loadState(supercollidaw::encodeState({ "{ K2A.ar(SuperColliDAW.beats / 1000) }.play;\n", "", {} }));
    instance.waitForSignal([](const std::vector<float>& signal) { return std::ranges::all_of(signal, [](float sample) { return sample > 0.05f; }); });
    const double playingError = worstBeatsError(instance.runSilence(1.0, kMaxFrames), transport.songBeats, 90.0, 48000.0);
    transport.playing = false;
    const double stoppedError = worstBeatsError(instance.runSilence(1.0, kMaxFrames), transport.songBeats, 90.0, 48000.0);
    transport.playing = true;
    transport.songBeats = 200.0;
    const double relocatedError = worstBeatsError(instance.runSilence(1.0, kMaxFrames), 200.0, 90.0, 48000.0);
    std::printf("  SuperColliDAW.beats worst error: %.4f beats playing, %.4f stopped, %.4f after jumping to beat 200\n", playingError, stoppedError, relocatedError);
    check(playingError < 0.01, "SuperColliDAW.beats is the DAW's position while it plays");
    check(stoppedError < 0.01, "SuperColliDAW.beats keeps counting at the DAW's tempo while it is stopped");
    check(relocatedError < 0.01, "SuperColliDAW.beats jumps with the DAW when it relocates");
}

void testParameters(const clap_plugin_factory* factory) {
    const std::string code = "{ SinOsc.ar(SuperColliDAW.kr(0, \\pitch, [200, 800, \\exp]), 0, 0.1 * (1 - (In.kr(5) * 0.001))) }.play;\n"
                             "s.bind { { SuperColliDAW.kr(2, \\bundled); Silent.ar }.play };\n"
                             "{ SuperColliDAW.kr(3, \\octave, 0, 3, step: 1, start: 1) + SuperColliDAW.kr(4, \\freq, high: 2000); Silent.ar }.play;\n";
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    check(instance.waitForParameter(0, "pitch"), "SuperColliDAW.kr(0, \\pitch) shows parameter 0 named pitch");
    check(instance.waitForParameter(5, "In.kr(5)"), "In.kr(5) shows parameter 5");
    check(instance.waitForParameter(2, "bundled"), "SuperColliDAW.kr inside s.bind shows its parameter");
    check(instance.parameterInfo(1).flags & CLAP_PARAM_IS_HIDDEN, "unused parameters stay hidden");
    check(instance.waitForParameter(3, "octave") && instance.waitForParameter(4, "freq"), "named arguments declare parameters");
    char octaveText[64];
    char freqText[64];
    instance.params()->value_to_text(instance.clapPlugin(), 3, 0.5, octaveText, sizeof(octaveText));
    instance.params()->value_to_text(instance.clapPlugin(), 4, 0.5, freqText, sizeof(freqText));
    std::printf("  octave at 0.5 shows \"%s\", starts at %.4f; freq at 0.5 shows \"%s\"\n", octaveText, instance.parameterInfo(3).default_value, freqText);
    check(std::string(octaveText) == "2" && std::fabs(instance.parameterInfo(3).default_value - 1.0 / 3.0) < 1e-6, "kr(3, \\octave, 0, 3, step: 1, start: 1) snaps to whole numbers and starts at 1");
    check(std::string(freqText) == "200 Hz", "kr(4, \\freq, high: 2000) keeps the curve and unit of \\freq with a new top");
    std::printf("  default: %.2f Hz\n", instance.waitForFrequency(200.0));

    instance.setParameter(0, 1.0);
    const double atMax = instance.waitForFrequency(800.0);
    std::printf("  parameter at 1.0: %.2f Hz\n", atMax);
    check(std::fabs(atMax - 800.0) <= 4.0, "a host value change reaches In.kr through the spec");

    instance.modulateParameter(0, -0.5);
    const double modulated = instance.waitForFrequency(400.0);
    std::printf("  modulated by -0.5: %.2f Hz\n", modulated);
    check(std::fabs(modulated - 400.0) <= 4.0, "host modulation offsets the value");

    char text[64];
    instance.params()->value_to_text(instance.clapPlugin(), 0, 0.5, text, sizeof(text));
    double parsed = 0.0;
    instance.params()->text_to_value(instance.clapPlugin(), 0, "400", &parsed);
    std::printf("  0.5 displays as \"%s\", \"400\" parses to %.4f\n", text, parsed);
    check(std::string(text) == "400" && std::fabs(parsed - 0.5) < 1e-9, "values display and parse in spec units");

    const std::string saved = instance.saveState();
    Instance restored(factory, 48000.0);
    restored.loadState(saved);
    double restoredValue = 0.0;
    restored.params()->get_value(restored.clapPlugin(), 0, &restoredValue);
    check(restoredValue == 1.0 && std::string(restored.parameterInfo(0).name) == "pitch", "parameter values and names are saved in the project");
    check(gParameterRescans > 0, "the host is told to rescan parameters");
}

}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <plugin.clap>\n", argv[0]);
        return 2;
    }
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 2;
    }
    char userDir[] = "/tmp/supercollidaw-test-XXXXXX";
    setenv("SUPERCOLLIDAW_USER_DIR", mkdtemp(userDir), 1);
    gCodeFile = std::filesystem::path(userDir) / "default.scd";
    writeCode(kTestSineHz);

    auto* entry = static_cast<const clap_plugin_entry*>(dlsym(library, "clap_entry"));
    check(entry && entry->init(argv[1]), "clap_entry initialises");
    auto* factory = static_cast<const clap_plugin_factory*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));

    testSineFollowsHostSampleRate(factory, 48000.0);
    testSineFollowsHostSampleRate(factory, 44100.0);
    testTrackInputPassesThroughWithReportedLatency(factory);
    testTwoInstancesRunTogether(factory);
    testStateRestoresCode(factory);
    testFileIsNeverLoadedOnItsOwn(factory);
    testReactivationKeepsServerNotifications(factory);
    testUnprocessedInstanceRecovers(factory);
    testStuckSclangDoesNotBlockTheHost(factory);
    testTrackMidiReachesMIDIdef(factory);
    testInstrumentPlaysTrackNotes(factory);
    testInstrumentPassesNoteDetails(factory);
    testInstrumentBendsPitch(factory);
    testInstrumentSustainPedal(factory);
    testInstrumentMode(factory, "\\mono", false);
    testInstrumentMode(factory, "\\legato", true);
    testInstrumentFollowsMpeBend(factory);
    testInstrumentFollowsMpeTimbreAndPressure(factory);
    testClockFollowsTransport(factory);
    testTransportUGens(factory);
    testPatternMidiReachesTheTrack(factory);
    testRunAllReleasesHeldMidiNotes(factory);
    testReactivationReleasesHeldMidiNotes(factory);
    testBrokenSynthStaysInsideThePlugin(factory);
    testParameters(factory);
    testExamplesRun(factory, std::filesystem::path(argv[1]).parent_path() / "SuperColliDAW" / "examples");
    entry->deinit();

    check(entry->init(argv[1]), "clap_entry initialises again after deinit");
    entry->deinit();
    std::filesystem::remove_all(userDir);
    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
