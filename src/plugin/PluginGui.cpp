#include "PluginGui.h"

#include "Plugin.h"

#include <algorithm>
#include <cstring>

namespace supercollidaw {

namespace {

constexpr uint32_t kDefaultWidth = 900;
constexpr uint32_t kDefaultHeight = 600;
constexpr uint32_t kMinWidth = 400;
constexpr uint32_t kMinHeight = 300;
constexpr uint32_t kFrameIntervalMs = 16;

bool isEmbeddedX11(const char* api, bool isFloating) { return !isFloating && api && !std::strcmp(api, CLAP_WINDOW_API_X11); }

}

const clap_plugin_gui PluginGui::kExtension = {
    .is_api_supported = [](const clap_plugin*, const char* api, bool isFloating) { return isEmbeddedX11(api, isFloating); },
    .get_preferred_api = [](const clap_plugin*, const char** api, bool* isFloating) {
        *api = CLAP_WINDOW_API_X11;
        *isFloating = false;
        return true;
    },
    .create = [](const clap_plugin* plugin, const char* api, bool isFloating) {
        return isEmbeddedX11(api, isFloating) && from(plugin).create();
    },
    .destroy = [](const clap_plugin* plugin) { from(plugin).destroy(); },
    .set_scale = [](const clap_plugin* plugin, double scale) { return from(plugin).setScale(scale); },
    .get_size = [](const clap_plugin* plugin, uint32_t* width, uint32_t* height) { return from(plugin).getSize(width, height); },
    .can_resize = [](const clap_plugin*) { return true; },
    .get_resize_hints = [](const clap_plugin*, clap_gui_resize_hints* hints) {
        *hints = { .can_resize_horizontally = true, .can_resize_vertically = true, .preserve_aspect_ratio = false };
        return true;
    },
    .adjust_size = [](const clap_plugin*, uint32_t* width, uint32_t* height) {
        *width = std::max(*width, kMinWidth);
        *height = std::max(*height, kMinHeight);
        return true;
    },
    .set_size = [](const clap_plugin* plugin, uint32_t width, uint32_t height) { return from(plugin).setSize(width, height); },
    .set_parent = [](const clap_plugin* plugin, const clap_window* window) { return from(plugin).setParent(window); },
    .set_transient = [](const clap_plugin*, const clap_window*) { return false; },
    .suggest_title = [](const clap_plugin*, const char*) {},
    .show = [](const clap_plugin* plugin) { return from(plugin).show(); },
    .hide = [](const clap_plugin* plugin) { return from(plugin).hide(); },
};

PluginGui::PluginGui(const clap_host* host, const clap_host_timer_support* hostTimer, EditorActions actions, const PostLog& postLog):
    mHost(host), mHostTimer(hostTimer), mWidth(kDefaultWidth), mHeight(kDefaultHeight), mView(std::move(actions), postLog) {}

PluginGui::~PluginGui() { destroy(); }

PluginGui& PluginGui::from(const clap_plugin* plugin) { return Plugin::gui(plugin); }

bool PluginGui::create() {
    if (!mHostTimer || !mHostTimer->register_timer)
        return false;
    return mHostTimer->register_timer(mHost, kFrameIntervalMs, &mFrameTimer);
}

void PluginGui::destroy() {
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

bool PluginGui::setSize(uint32_t width, uint32_t height) {
    mWidth = width;
    mHeight = height;
    if (mWindow)
        mWindow->setSize(width, height);
    return true;
}

bool PluginGui::setScale(double scale) {
    mScale = scale;
    if (mWindow)
        mWindow->setScale(scale);
    return true;
}

bool PluginGui::setParent(const clap_window* window) {
    const auto parent = static_cast<PuglNativeView>(window->x11);
    mWindow = std::make_unique<EditorWindow>(parent, mWidth, mHeight, mScale, [this] { mView.draw(); });
    if (mWindow->isRealized())
        return true;
    mWindow.reset();
    return false;
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
