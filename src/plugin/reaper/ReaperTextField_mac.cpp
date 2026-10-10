#include "ReaperTextField.h"

namespace supercollidaw {

// TODO: check in REAPER on macOS whether Space and Cmd+V reach the editor. reaper_plugin.h needs WDL's SWELL headers there.
ReaperTextField::ReaperTextField(const clap_host*, PuglNativeView view): mView(view) {}

ReaperTextField::~ReaperTextField() = default;

}
