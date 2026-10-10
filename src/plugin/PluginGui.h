#pragma once

#include "reaper/ReaperTextField.h"
#include "ui/EditorView.h"
#include "ui/EditorWindow.h"

#include <clap/clap.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace supercollidaw {

class PluginGui {
public:
    static const clap_plugin_gui kExtension;
    static const char* const kWindowApi;

    static const clap_plugin_posix_fd_support kPosixFdExtension;

    PluginGui(const clap_host* host, const clap_host_timer_support* hostTimer, const clap_host_posix_fd_support* hostFd, EditorActions actions,
        const PostLog& postLog, std::vector<Example> examples);
    ~PluginGui();
    PluginGui(const PluginGui&) = delete;
    PluginGui& operator=(const PluginGui&) = delete;

    bool onTimer(clap_id timerId);
    void onFd(int fd);
    void setCode(const std::string& code) { mView.setCode(code); }
    void replaceCode(const std::string& code) { mView.replaceCode(code); }
    void setStatus(EditorStatus status) { mView.setStatus(std::move(status)); }
    void setSettings(EditorSettings settings) { mView.setSettings(std::move(settings)); }
    void showCompletion(const Completion& completion) { mView.showCompletion(completion); }
    void showSignatureHelp(const SignatureHelp& help) { mView.showSignatureHelp(help); }

private:
    static PluginGui& from(const clap_plugin* plugin);
    static PuglNativeView nativeParent(const clap_window& window);

    bool create();
    void destroy();
    bool getSize(uint32_t* width, uint32_t* height) const;
    void adjustSize(uint32_t* width, uint32_t* height) const;
    bool setSize(uint32_t width, uint32_t height);
    bool setScale(double scale);
    bool setParent(const clap_window* window);
    bool show();
    bool hide();
    void registerEventFd();
    void unregisterEventFd();

    const clap_host* mHost;
    const clap_host_timer_support* mHostTimer;
    const clap_host_posix_fd_support* mHostFd;
    clap_id mFrameTimer = CLAP_INVALID_ID;
    int mEventFd = -1;
    uint32_t mWidth;
    uint32_t mHeight;
    std::optional<double> mHostScale;
    EditorView mView;
    std::unique_ptr<EditorWindow> mWindow;
    std::optional<ReaperTextField> mReaperTextField;
};

}
