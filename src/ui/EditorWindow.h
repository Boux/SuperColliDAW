#pragma once

#include "PuglClipboard.h"

#include <pugl/pugl.h>

#include <cstdint>
#include <functional>

struct ImGuiContext;
struct ImGuiIO;

namespace supercollidaw {

class EditorWindow {
public:
    using DrawContents = std::function<void()>;

    EditorWindow(PuglNativeView parent, uint32_t width, uint32_t height, double scale, DrawContents drawContents);
    ~EditorWindow();
    EditorWindow(const EditorWindow&) = delete;
    EditorWindow& operator=(const EditorWindow&) = delete;

    bool isRealized() const { return mRealized; }
    void setSize(uint32_t width, uint32_t height);
    void setScale(double scale);
    double systemScale() const;
    void show();
    void hide();
    void idle();
    void processEvents();
    int eventFd() const;

private:
    static PuglStatus onEvent(PuglView* view, const PuglEvent* event);

    void useSystemKeyRepeat(ImGuiIO& io);
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
    bool mRealized = false;
    bool mRendererStarted = false;
    PuglCursor mCursor = PUGL_CURSOR_ARROW;
    double mLastFrameTime = 0.0;
};

}
