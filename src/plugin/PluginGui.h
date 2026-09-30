#pragma once

#include "ui/EditorView.h"
#include "ui/EditorWindow.h"

#include <clap/clap.h>

#include <memory>
#include <string>

namespace supercollidaw {

class PluginGui {
public:
    static const clap_plugin_gui kExtension;

    PluginGui(const clap_host* host, const clap_host_timer_support* hostTimer, EditorActions actions, const PostLog& postLog);
    ~PluginGui();
    PluginGui(const PluginGui&) = delete;
    PluginGui& operator=(const PluginGui&) = delete;

    bool onTimer(clap_id timerId);
    void setCode(const std::string& code) { mView.setCode(code); }

private:
    static PluginGui& from(const clap_plugin* plugin);

    bool create();
    void destroy();
    bool getSize(uint32_t* width, uint32_t* height) const;
    bool setSize(uint32_t width, uint32_t height);
    bool setScale(double scale);
    bool setParent(const clap_window* window);
    bool show();
    bool hide();

    const clap_host* mHost;
    const clap_host_timer_support* mHostTimer;
    clap_id mFrameTimer = CLAP_INVALID_ID;
    uint32_t mWidth;
    uint32_t mHeight;
    double mScale = 1.0;
    EditorView mView;
    std::unique_ptr<EditorWindow> mWindow;
};

}
