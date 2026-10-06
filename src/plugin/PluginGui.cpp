#include "PluginGui.h"

#include "Plugin.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace supercollidaw {

namespace {

constexpr uint32_t kDefaultWidth = 900;
constexpr uint32_t kDefaultHeight = 600;
constexpr uint32_t kMinWidth = 400;
constexpr uint32_t kMinHeight = 300;
constexpr uint32_t kFrameIntervalMs = 16;

bool isEmbeddedNative(const char* api, bool isFloating) { return !isFloating && api && !std::strcmp(api, PluginGui::kWindowApi); }

}

const clap_plugin_gui PluginGui::kExtension = {
    .is_api_supported = [](const clap_plugin*, const char* api, bool isFloating) { return isEmbeddedNative(api, isFloating); },
    .get_preferred_api = [](const clap_plugin*, const char** api, bool* isFloating) {
        *api = kWindowApi;
        *isFloating = false;
        return true;
    },
    .create = [](const clap_plugin* plugin, const char* api, bool isFloating) {
        return isEmbeddedNative(api, isFloating) && from(plugin).create();
    },
    .destroy = [](const clap_plugin* plugin) { from(plugin).destroy(); },
    .set_scale = [](const clap_plugin* plugin, double scale) { return from(plugin).setScale(scale); },
    .get_size = [](const clap_plugin* plugin, uint32_t* width, uint32_t* height) { return from(plugin).getSize(width, height); },
    .can_resize = [](const clap_plugin*) { return true; },
    .get_resize_hints = [](const clap_plugin*, clap_gui_resize_hints* hints) {
        *hints = { .can_resize_horizontally = true, .can_resize_vertically = true, .preserve_aspect_ratio = false };
        return true;
    },
    .adjust_size = [](const clap_plugin* plugin, uint32_t* width, uint32_t* height) {
        from(plugin).adjustSize(width, height);
        return true;
    },
    .set_size = [](const clap_plugin* plugin, uint32_t width, uint32_t height) { return from(plugin).setSize(width, height); },
    .set_parent = [](const clap_plugin* plugin, const clap_window* window) { return from(plugin).setParent(window); },
    .set_transient = [](const clap_plugin*, const clap_window*) { return false; },
    .suggest_title = [](const clap_plugin*, const char*) {},
    .show = [](const clap_plugin* plugin) { return from(plugin).show(); },
    .hide = [](const clap_plugin* plugin) { return from(plugin).hide(); },
};

const clap_plugin_posix_fd_support PluginGui::kPosixFdExtension = {
    .on_fd = [](const clap_plugin* plugin, int fd, clap_posix_fd_flags_t) { from(plugin).onFd(fd); },
};

PluginGui::PluginGui(const clap_host* host, const clap_host_timer_support* hostTimer, const clap_host_posix_fd_support* hostFd,
    EditorActions actions, const PostLog& postLog, std::vector<Example> examples):
    mHost(host),
    mHostTimer(hostTimer),
    mHostFd(hostFd),
    mWidth(kDefaultWidth),
    mHeight(kDefaultHeight),
    mView(std::move(actions), postLog, std::move(examples)) {}

PluginGui::~PluginGui() { destroy(); }

PluginGui& PluginGui::from(const clap_plugin* plugin) { return Plugin::gui(plugin); }

bool PluginGui::create() {
    if (!mHostTimer || !mHostTimer->register_timer)
        return false;
    return mHostTimer->register_timer(mHost, kFrameIntervalMs, &mFrameTimer);
}

void PluginGui::destroy() {
    unregisterEventFd();
    mWindow.reset();
    if (mFrameTimer == CLAP_INVALID_ID)
        return;
    mHostTimer->unregister_timer(mHost, mFrameTimer);
    mFrameTimer = CLAP_INVALID_ID;
}

bool PluginGui::getSize(uint32_t* width, uint32_t* height) const {
    *width = mWidth;
    *height = mHeight;
    return true;
}

void PluginGui::adjustSize(uint32_t* width, uint32_t* height) const {
    const double scale = mHostScale.value_or(1.0);
    *width = std::max(*width, static_cast<uint32_t>(std::lround(kMinWidth * scale)));
    *height = std::max(*height, static_cast<uint32_t>(std::lround(kMinHeight * scale)));
}

bool PluginGui::setSize(uint32_t width, uint32_t height) {
    mWidth = width;
    mHeight = height;
    if (mWindow)
        mWindow->setSize(width, height);
    return true;
}

// X11 and Win32 sizes are physical pixels, so the size follows the scale to keep the editor the same size on screen.
bool PluginGui::setScale(double scale) {
    const double change = scale / mHostScale.value_or(1.0);
    mWidth = static_cast<uint32_t>(std::lround(mWidth * change));
    mHeight = static_cast<uint32_t>(std::lround(mHeight * change));
    mHostScale = scale;
    if (mWindow)
        mWindow->setScale(scale);
    return true;
}

bool PluginGui::setParent(const clap_window* window) {
    mWindow = std::make_unique<EditorWindow>(nativeParent(*window), mWidth, mHeight, mHostScale.value_or(1.0), [this] { mView.draw(); });
    if (!mWindow->isRealized()) {
        mWindow.reset();
        return false;
    }
    // CLAP hosts set the scale before embedding; one that does not leaves it to the plugin, so follow the system's.
    if (!mHostScale)
        mWindow->setScale(mWindow->systemScale());
    registerEventFd();
    return true;
}

void PluginGui::registerEventFd() {
    const int fd = mWindow->eventFd();
    if (fd < 0 || !mHostFd || !mHostFd->register_fd || !mHostFd->register_fd(mHost, fd, CLAP_POSIX_FD_READ))
        return;
    mEventFd = fd;
}

void PluginGui::unregisterEventFd() {
    if (mEventFd < 0)
        return;
    mHostFd->unregister_fd(mHost, mEventFd);
    mEventFd = -1;
}

void PluginGui::onFd(int fd) {
    if (mWindow && fd == mEventFd)
        mWindow->processEvents();
}

bool PluginGui::show() {
    if (!mWindow)
        return false;
    mWindow->show();
    return true;
}

bool PluginGui::hide() {
    if (!mWindow)
        return false;
    mWindow->hide();
    return true;
}

bool PluginGui::onTimer(clap_id timerId) {
    if (timerId != mFrameTimer || !mWindow)
        return false;
    mWindow->idle();
    return true;
}

}
