#include "ReaperTextField.h"

namespace supercollidaw {

// X11 sends keys straight to the editor's window, so REAPER's shortcuts never see them.
ReaperTextField::ReaperTextField(const clap_host*, PuglNativeView view): mView(view) {}

ReaperTextField::~ReaperTextField() = default;

}
