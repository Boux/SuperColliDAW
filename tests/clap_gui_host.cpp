#include <clap/clap.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <poll.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#include <string>
#include <thread>

namespace {

constexpr clap_id kFirstTimerId = 1;
constexpr auto kFrameInterval = std::chrono::milliseconds(16);
constexpr double kSampleRate = 48000.0;
constexpr uint32_t kBlockFrames = 512;
constexpr uint32_t kChannels = 2;

std::set<clap_id> gTimers;
std::set<int> gFds;
int gFdCallbacks = 0;
std::atomic<bool> gCallbackRequested = false;

const clap_host_posix_fd_support kHostFdSupport = {
    .register_fd = [](const clap_host*, int fd, clap_posix_fd_flags_t) { return gFds.insert(fd).second; },
    .modify_fd = [](const clap_host*, int fd, clap_posix_fd_flags_t) { return gFds.count(fd) == 1; },
    .unregister_fd = [](const clap_host*, int fd) { return gFds.erase(fd) == 1; },
};

const clap_host_timer_support kHostTimerSupport = {
    .register_timer = [](const clap_host*, uint32_t, clap_id* timerId) {
        *timerId = kFirstTimerId + static_cast<clap_id>(gTimers.size());
        gTimers.insert(*timerId);
        return true;
    },
    .unregister_timer = [](const clap_host*, clap_id timerId) { return gTimers.erase(timerId) == 1; },
};

const clap_host kHost = {
    .clap_version = CLAP_VERSION_INIT,
    .host_data = nullptr,
    .name = "supercollidaw-gui-host",
    .vendor = "",
    .url = "",
    .version = "0",
    .get_extension = [](const clap_host*, const char* id) -> const void* {
        if (std::string(id) == CLAP_EXT_POSIX_FD_SUPPORT)
            return &kHostFdSupport;
        return std::string(id) == CLAP_EXT_TIMER_SUPPORT ? &kHostTimerSupport : nullptr;
    },
    .request_restart = [](const clap_host*) {},
    .request_process = [](const clap_host*) {},
    .request_callback = [](const clap_host*) { gCallbackRequested = true; },
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

float processBlock(const clap_plugin* plugin, std::vector<float>& silence, std::vector<float>& left, std::vector<float>& right) {
    float* inChannels[kChannels] = { silence.data(), silence.data() };
    float* outChannels[kChannels] = { left.data(), right.data() };
    clap_audio_buffer input = { .data32 = inChannels, .data64 = nullptr, .channel_count = kChannels, .latency = 0, .constant_mask = 0 };
    clap_audio_buffer output = { .data32 = outChannels, .data64 = nullptr, .channel_count = kChannels, .latency = 0, .constant_mask = 0 };
    clap_process process = {
        .steady_time = -1,
        .frames_count = kBlockFrames,
        .transport = nullptr,
        .audio_inputs = &input,
        .audio_outputs = &output,
        .audio_inputs_count = 1,
        .audio_outputs_count = 1,
        .in_events = &kNoInputEvents,
        .out_events = &kDiscardOutputEvents,
    };
    plugin->process(plugin, &process);
    float peak = 0.f;
    for (float sample : left)
        peak = std::max(peak, std::fabs(sample));
    return peak;
}

std::thread startAudio(const clap_plugin* plugin, std::atomic<bool>& running, std::atomic<float>& peak) {
    return std::thread([plugin, &running, &peak] {
        std::vector<float> silence(kBlockFrames, 0.f), left(kBlockFrames), right(kBlockFrames);
        const auto period = std::chrono::duration<double>(kBlockFrames / kSampleRate);
        auto next = std::chrono::steady_clock::now();
        while (running) {
            peak = std::max(peak.load(), processBlock(plugin, silence, left, right));
            next += std::chrono::duration_cast<std::chrono::steady_clock::duration>(period);
            std::this_thread::sleep_until(next);
        }
    });
}

void writePpm(Display* display, Window window, uint32_t width, uint32_t height, const char* path) {
    XImage* image = XGetImage(display, window, 0, 0, width, height, AllPlanes, ZPixmap);
    std::ofstream file(path, std::ios::binary);
    file << "P6\n" << width << " " << height << "\n255\n";
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const unsigned long pixel = XGetPixel(image, x, y);
            const char rgb[3] = { char((pixel >> 16) & 0xff), char((pixel >> 8) & 0xff), char(pixel & 0xff) };
            file.write(rgb, 3);
        }
    }
    XDestroyImage(image);
}

}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: %s <plugin.clap> <seconds> <screenshot.ppm> [scale]\n", argv[0]);
        return 2;
    }
    const double seconds = std::atof(argv[2]);
    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    auto* entry = static_cast<const clap_plugin_entry*>(dlsym(library, "clap_entry"));
    entry->init(argv[1]);
    auto* factory = static_cast<const clap_plugin_factory*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
    const clap_plugin* plugin = factory->create_plugin(factory, &kHost, factory->get_plugin_descriptor(factory, 0)->id);
    plugin->init(plugin);
    plugin->activate(plugin, kSampleRate, 1, kBlockFrames);
    plugin->start_processing(plugin);
    std::atomic<bool> audioRunning = true;
    std::atomic<float> peak = 0.f;
    std::thread audio = startAudio(plugin, audioRunning, peak);

    auto* gui = static_cast<const clap_plugin_gui*>(plugin->get_extension(plugin, CLAP_EXT_GUI));
    auto* timer = static_cast<const clap_plugin_timer_support*>(plugin->get_extension(plugin, CLAP_EXT_TIMER_SUPPORT));
    auto* fdSupport = static_cast<const clap_plugin_posix_fd_support*>(plugin->get_extension(plugin, CLAP_EXT_POSIX_FD_SUPPORT));
    if (!gui || !gui->create(plugin, CLAP_WINDOW_API_X11, false)) {
        std::fprintf(stderr, "plugin gui could not be created\n");
        return 1;
    }
    if (argc > 4)
        gui->set_scale(plugin, std::atof(argv[4]));
    uint32_t width = 0, height = 0;
    gui->get_size(plugin, &width, &height);

    Display* display = XOpenDisplay(nullptr);
    Window parent = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, width, height, 0, 0, 0);
    XStoreName(display, parent, "SuperColliDAW GUI host");
    XMapWindow(display, parent);
    XSync(display, False);

    const clap_window window = { .api = CLAP_WINDOW_API_X11, .x11 = parent };
    if (!gui->set_parent(plugin, &window) || !gui->show(plugin)) {
        std::fprintf(stderr, "plugin gui could not be embedded\n");
        return 1;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        for (clap_id id : std::set<clap_id>(gTimers))
            timer->on_timer(plugin, id);
        if (gCallbackRequested.exchange(false))
            plugin->on_main_thread(plugin);
        for (int fd : std::set<int>(gFds)) {
            pollfd request = { .fd = fd, .events = POLLIN, .revents = 0 };
            if (poll(&request, 1, 0) > 0 && (request.revents & POLLIN)) {
                fdSupport->on_fd(plugin, fd, CLAP_POSIX_FD_READ);
                ++gFdCallbacks;
            }
        }
        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
        }
        std::this_thread::sleep_for(kFrameInterval);
    }

    writePpm(display, parent, width, height, argv[3]);
    gui->hide(plugin);
    gui->destroy(plugin);
    audioRunning = false;
    audio.join();
    plugin->stop_processing(plugin);
    plugin->deactivate(plugin);
    plugin->destroy(plugin);
    entry->deinit();
    XDestroyWindow(display, parent);
    XCloseDisplay(display);
    std::printf("gui ran for %.1f s at %ux%u, output peak %.3f, %d fd callbacks, screenshot in %s\n", seconds, width, height, peak.load(),
        gFdCallbacks, argv[3]);
    return 0;
}
