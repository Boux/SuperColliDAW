#pragma once

#include "EditorSettings.h"
#include "FixedFonts.h"
#include "PuglClipboard.h"

#include <pugl/pugl.h>

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

struct ImFont;
struct ImGuiContext;
struct ImGuiIO;

namespace supercollidaw {

struct BundledFont;

class EditorWindow {
public:
    using DrawContents = std::function<void(bool hasFocus)>;

    EditorWindow(PuglNativeView parent, uint32_t width, uint32_t height, double scale, DrawContents drawContents);
    ~EditorWindow();
    EditorWindow(const EditorWindow&) = delete;
    EditorWindow& operator=(const EditorWindow&) = delete;

    bool isRealized() const { return mRealized; }
    void setSize(uint32_t width, uint32_t height);
    void setScale(double scale);
    double systemScale() const;
    void setSettings(const EditorSettings& settings);
    FixedFonts fixedFonts() const;
    void show();
    void hide();
    void idle();
    void processEvents();
    int eventFd() const;

private:
    struct LoadedFont {
        const BundledFont* source;
        ImFont* font;
        float baseSize;
    };

    static PuglStatus onEvent(PuglView* view, const PuglEvent* event);

    void addFonts(ImGuiIO& io);
    const LoadedFont& loadedFont(std::string_view name) const;
    void applyStyle();

    void useSystemKeyRepeat(ImGuiIO& io);
    void takeFocusOnActivation();
    PuglStatus handle(const PuglEvent& event);
    PuglStatus startRenderer();
    PuglStatus stopRenderer();
    PuglStatus drawFrame();
    void updateCursor();

    DrawContents mDrawContents;
    PuglWorld* mWorld;
    PuglView* mView;
    PuglClipboard mClipboard;
    ImGuiContext* mImGui;
    double mScale;
    std::vector<LoadedFont> mFonts;
    ImFont* mIconFont = nullptr;
    EditorSettings mSettings;
    bool mStyleOutdated = true;
    bool mRealized = false;
    // ImGui starts out assuming the window has focus.
    bool mHasFocus = true;
    PuglNativeView mActiveWindow = 0;
    PuglCursor mCursor = PUGL_CURSOR_ARROW;
    double mLastFrameTime = 0.0;
};

}
