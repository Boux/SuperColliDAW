#include "plugin/state/PluginState.h"

#include <clap/clap.h>

#include <dlfcn.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
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

const clap_ostream* appendingStream(std::string& bytes) {
    static clap_ostream stream;
    stream = {
        .ctx = &bytes,
        .write = [](const clap_ostream* s, const void* buffer, uint64_t size) -> int64_t {
            static_cast<std::string*>(s->ctx)->append(static_cast<const char*>(buffer), size);
            return static_cast<int64_t>(size);
        },
    };
    return &stream;
}

struct ReadCursor {
    const std::string& bytes;
    size_t position = 0;
};

const clap_istream* readingStream(ReadCursor& cursor) {
    static clap_istream stream;
    stream = {
        .ctx = &cursor,
        .read = [](const clap_istream* s, void* buffer, uint64_t size) -> int64_t {
            auto* c = static_cast<ReadCursor*>(s->ctx);
            const size_t count = std::min<size_t>(size, c->bytes.size() - c->position);
            std::memcpy(buffer, c->bytes.data() + c->position, count);
            c->position += count;
            return static_cast<int64_t>(count);
        },
    };
    return &stream;
}

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

struct ParameterEvents {
    std::vector<clap_event_param_value> values;
    std::vector<clap_event_param_mod> modulations;
    std::vector<const clap_event_header*> headers;

    const clap_input_events* list() {
        headers.clear();
        for (const auto& event : values)
            headers.push_back(&event.header);
        for (const auto& event : modulations)
            headers.push_back(&event.header);
        static clap_input_events events;
        events = {
            .ctx = this,
            .size = [](const clap_input_events* e) { return static_cast<uint32_t>(static_cast<ParameterEvents*>(e->ctx)->headers.size()); },
            .get = [](const clap_input_events* e, uint32_t index) { return static_cast<ParameterEvents*>(e->ctx)->headers[index]; },
        };
        return &events;
    }

    void clear() {
        values.clear();
        modulations.clear();
    }
};

clap_event_header eventHeader(uint16_t type, uint32_t size) {
    return { .size = size, .time = 0, .space_id = CLAP_CORE_EVENT_SPACE_ID, .type = type, .flags = 0 };
}

const clap_output_events kDiscardOutputEvents = {
    .ctx = nullptr,
    .try_push = [](const clap_output_events*, const clap_event_header_t*) { return true; },
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

    bool waitForSound() {
        const std::vector<float> silence(kMaxFrames, 0.f);
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (peak(run(silence, kMaxFrames)) > kTestSineAmp * 0.5f)
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
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

    double waitForFrequency(double expectedHz) {
        double frequency = 0.0;
        const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
        while (std::fabs(frequency - expectedHz) > 4.0 && std::chrono::steady_clock::now() < deadline)
            frequency = estimateFrequency(runSilence(0.5, kMaxFrames), mSampleRate);
        return frequency;
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
        clap_process process = {
            .steady_time = -1,
            .frames_count = frames,
            .transport = nullptr,
            .audio_inputs = &input,
            .audio_outputs = &output,
            .audio_inputs_count = 1,
            .audio_outputs_count = 1,
            .in_events = mEvents.list(),
            .out_events = &kDiscardOutputEvents,
        };
        mPlugin->process(mPlugin, &process);
        mEvents.clear();
    }

    double mSampleRate;
    ParameterEvents mEvents;
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

void testLinkedFileRerunsWhenSaved(const clap_plugin_factory* factory) {
    const std::filesystem::path linked = gCodeFile.parent_path() / "linked.scd";
    writeCode(linked, 660.0);
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    check(instance.loadState(supercollidaw::encodeState({ sineCode(660.0), linked.string() })), "a state linking a file loads");
    std::printf("  linked file: frequency %.2f Hz\n", instance.waitForFrequency(660.0));
    writeCode(linked, 880.0);
    instance.fireTimer();
    const double frequency = instance.waitForFrequency(880.0);
    std::printf("  after saving the linked file: frequency %.2f Hz\n", frequency);
    check(std::fabs(frequency - 880.0) <= 4.0, "saving the linked file re-runs it");
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

void testParameters(const clap_plugin_factory* factory) {
    const std::string code = "{ SinOsc.ar(SuperColliDAW.kr(0, \\pitch, [200, 800, \\exp]), 0, 0.1 * (1 - (In.kr(5) * 0.001))) }.play;\n"
                             "s.bind { { SuperColliDAW.kr(2, \\bundled); Silent.ar }.play };\n";
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    instance.loadState(supercollidaw::encodeState({ code, "", {} }));
    check(instance.waitForParameter(0, "pitch"), "SuperColliDAW.kr(0, \\pitch) shows parameter 0 named pitch");
    check(instance.waitForParameter(5, "In.kr(5)"), "In.kr(5) shows parameter 5");
    check(instance.waitForParameter(2, "bundled"), "SuperColliDAW.kr inside s.bind shows its parameter");
    check(instance.parameterInfo(1).flags & CLAP_PARAM_IS_HIDDEN, "unused parameters stay hidden");
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
    testLinkedFileRerunsWhenSaved(factory);
    testReactivationKeepsServerNotifications(factory);
    testStuckSclangDoesNotBlockTheHost(factory);
    testParameters(factory);
    entry->deinit();

    check(entry->init(argv[1]), "clap_entry initialises again after deinit");
    testSineFollowsHostSampleRate(factory, 48000.0);
    entry->deinit();
    std::filesystem::remove_all(userDir);
    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
