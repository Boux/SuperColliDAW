#include "CodeFlash.h"

#include <algorithm>

namespace supercollidaw {

namespace {

constexpr std::chrono::duration<float> kDuration = std::chrono::milliseconds(600);
constexpr float kPeakAlpha = 0.6f;

ImU32 flashColor(float alpha) { return IM_COL32(0xc0, 0x7f, 0x00, static_cast<int>(alpha * 255.f)); }

}

void CodeFlash::start(TextEditor& editor, LineRange lines) {
    if (mEditor)
        mEditor->ClearMarkers();
    mEditor = &editor;
    mLines = lines;
    mStart = std::chrono::steady_clock::now();
}

void CodeFlash::update() {
    if (!mEditor)
        return;
    mEditor->ClearMarkers();
    const float progress = (std::chrono::steady_clock::now() - mStart) / kDuration;
    if (progress >= 1.f) {
        mEditor = nullptr;
        return;
    }
    const ImU32 color = flashColor(kPeakAlpha * (1.f - progress * progress * progress));
    const size_t last = std::min(mLines.last, mEditor->GetLineCount() - 1);
    for (size_t line = mLines.first; line <= last; ++line)
        mEditor->AddMarker(line, 0, color, "", "");
}

}
