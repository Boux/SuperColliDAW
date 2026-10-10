#pragma once

#include <clap/clap.h>
#include <pugl/pugl.h>

namespace supercollidaw {

// REAPER runs its shortcuts on keys typed into a plugin's window unless the window is a text field, so Space started playback and Ctrl+V pasted in REAPER.
class ReaperTextField {
public:
    ReaperTextField(const clap_host* host, PuglNativeView view);
    ~ReaperTextField();
    ReaperTextField(const ReaperTextField&) = delete;
    ReaperTextField& operator=(const ReaperTextField&) = delete;

private:
    using Register = int (*)(const char* name, void* info);

    Register mRegister = nullptr;
    PuglNativeView mView;
};

}
