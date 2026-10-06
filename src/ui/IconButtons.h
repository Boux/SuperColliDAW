#pragma once

#include "EditorSettings.h"

struct ImFont;

namespace supercollidaw {

constexpr float kIconSize = 15.f;

// Icons follow the text size setting from one base size, whatever the text font's own default size.
class IconButtons {
public:
    explicit IconButtons(const EditorSettings& settings): mSettings(settings) {}

    void setFont(ImFont* font) { mFont = font; }
    bool button(const char* icon, const char* tooltip) const;
    float width() const;

private:
    float size() const { return kIconSize * static_cast<float>(mSettings.fontSizePercent) / 100.f; }

    const EditorSettings& mSettings;
    ImFont* mFont = nullptr;
};

}
