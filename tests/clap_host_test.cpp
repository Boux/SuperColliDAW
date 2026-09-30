#include <clap/clap.h>

#include <dlfcn.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

void writeCode(double sineHz) {
    std::ofstream(gCodeFile) << "{ SoundIn.ar([0, 1]) + SinOsc.ar(" << sineHz << ", 0, " << kTestSineAmp << ") }.play;\n";
}

const clap_host_timer_support kHostTimerSupport = {
    .register_timer = [](const clap_host*, uint32_t, clap_id* timerId) {
        *timerId = kTimerId;
        return true;
    },
    .unregister_timer = [](const clap_host*, clap_id) { return true; },
};

const clap_host kHost = {
    .clap_version = CLAP_VERSION_INIT,
    .host_data = nullptr,
    .name = "supercollidaw-test-host",
    .vendor = "",
    .url = "",
    .version = "0",
    .get_extension = [](const clap_host*, const char* id) -> const void* {
        return std::string(id) == CLAP_EXT_TIMER_SUPPORT ? &kHostTimerSupport : nullptr;
    },
    .request_restart = [](const clap_host*) {},
    .request_process = [](const clap_host*) {},
    .request_callback = [](const clap_host*) {},
};

const clap_input_events kNoInputEvents = {
    .ctx = nullptr,
    .size = [](const clap_input_events*) -> uint32_t { return 0; },
    .get = [](const clap_input_events*, uint32_t) -> const clap_event_header_t* { return nullptr; },
};

const clap_output_events kDiscardOutputEvents = {
    .ctx = nullptr,
    .try_push = [](const clap_output_events*, const clap_event_header_t*) { return true; },
};

int gFailures = 0;

void check(bool condition, const char* what) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", what);
    gFailures += condition ? 0 : 1;
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

    std::vector<float> run(const std::vector<float>& input, uint32_t framesPerCall) {
        std::vector<float> left(input.size()), right(input.size());
        for (size_t offset = 0; offset < input.size(); offset += framesPerCall) {
            const uint32_t frames = std::min<size_t>(framesPerCall, input.size() - offset);
            processCall(input.data() + offset, left.data() + offset, right.data() + offset, frames);
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
            .in_events = &kNoInputEvents,
            .out_events = &kDiscardOutputEvents,
        };
        mPlugin->process(mPlugin, &process);
    }

    double mSampleRate;
    const clap_plugin* mPlugin = nullptr;
    bool mActive = false;
};

double estimateFrequency(const std::vector<float>& signal, double sampleRate) {
    int crossings = 0;
    for (size_t i = 1; i < signal.size(); ++i)
        crossings += (signal[i - 1] < 0.f && signal[i] >= 0.f) ? 1 : 0;
    return crossings * sampleRate / signal.size();
}

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

void testSavingTheCodeFileRerunsIt(const clap_plugin_factory* factory) {
    Instance instance(factory, 48000.0);
    instance.waitForSound();
    writeCode(880.0);
    instance.fireTimer();

    double frequency = 0.0;
    const auto deadline = std::chrono::steady_clock::now() + kSclangStartTimeout;
    while (std::fabs(frequency - 880.0) > 4.0 && std::chrono::steady_clock::now() < deadline)
        frequency = estimateFrequency(instance.runSilence(0.5, kMaxFrames), instance.sampleRate());
    std::printf("  after saving: frequency %.2f Hz\n", frequency);
    check(std::fabs(frequency - 880.0) <= 4.0, "saving the code file re-runs it");
    writeCode(kTestSineHz);
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
    testSavingTheCodeFileRerunsIt(factory);
    entry->deinit();

    check(entry->init(argv[1]), "clap_entry initialises again after deinit");
    testSineFollowsHostSampleRate(factory, 48000.0);
    entry->deinit();
    std::filesystem::remove_all(userDir);
    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
