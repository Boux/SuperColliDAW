#include "EditorWindow.h"

#include "PuglImGuiInput.h"

#include <dejavu.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <cstdio>
#include <imgui_impl_opengl3.h>
#include <pugl/gl.h>

#include <algorithm>

namespace supercollidaw {

namespace {

constexpr char kGlslVersion[] = "#version 330 core";
constexpr double kMinFrameTime = 1.0 / 1000.0;
constexpr float kFontSize = 15.f;

class ScopedImGuiContext {
public:
    explicit ScopedImGuiContext(ImGuiContext* context): mPrevious(ImGui::GetCurrentContext()) { ImGui::SetCurrentContext(context); }
    ~ScopedImGuiContext() { ImGui::SetCurrentContext(mPrevious); }

private:
    ImGuiContext* mPrevious;
};

void addCodeFont(ImGuiIO& io) { io.Fonts->AddFontFromMemoryCompressedTTF(dejavu, dejavuSize, kFontSize); }

}

EditorWindow::EditorWindow(PuglNativeView parent, uint32_t width, uint32_t height, double scale, DrawContents drawContents):
    mDrawContents(std::move(drawContents)),
    mWorld(puglNewWorld(PUGL_MODULE, 0)),
    mView(puglNewView(mWorld)),
    mClipboard(mView),
    mImGui(ImGui::CreateContext()) {
    ScopedImGuiContext context(mImGui);
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().BackendPlatformName = "pugl";
    addCodeFont(ImGui::GetIO());
    mClipboard.install();
    setScale(scale);

    puglSetWorldString(mWorld, PUGL_CLASS_NAME, "SuperColliDAW");
    puglSetHandle(mView, this);
    puglSetEventFunc(mView, onEvent);
    puglSetBackend(mView, puglGlBackend());
    puglSetViewHint(mView, PUGL_CONTEXT_API, PUGL_OPENGL_API);
    puglSetViewHint(mView, PUGL_CONTEXT_VERSION_MAJOR, 3);
    puglSetViewHint(mView, PUGL_CONTEXT_VERSION_MINOR, 3);
    puglSetViewHint(mView, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
    puglSetViewHint(mView, PUGL_DOUBLE_BUFFER, PUGL_TRUE);
    puglSetViewHint(mView, PUGL_RESIZABLE, PUGL_TRUE);
    puglSetSizeHint(mView, PUGL_DEFAULT_SIZE, width, height);
    puglSetParent(mView, parent);
    mRealized = puglRealize(mView) == PUGL_SUCCESS;
}

EditorWindow::~EditorWindow() {
    puglUnrealize(mView);
    puglFreeView(mView);
    puglFreeWorld(mWorld);
    ImGui::DestroyContext(mImGui);
}

void EditorWindow::setSize(uint32_t width, uint32_t height) { puglSetSizeHint(mView, PUGL_CURRENT_SIZE, width, height); }

void EditorWindow::setScale(double scale) {
    ScopedImGuiContext context(mImGui);
    ImGuiStyle style;
    ImGui::StyleColorsDark(&style);
    style.FontSizeBase = kFontSize;
    style.ScaleAllSizes(static_cast<float>(scale));
    style.FontScaleDpi = static_cast<float>(scale);
    ImGui::GetStyle() = style;
}

void EditorWindow::show() { puglShow(mView, PUGL_SHOW_PASSIVE); }

void EditorWindow::hide() { puglHide(mView); }

void EditorWindow::idle() {
    puglObscureView(mView);
    puglUpdate(mWorld, 0.0);
}

PuglStatus EditorWindow::onEvent(PuglView* view, const PuglEvent* event) {
    return static_cast<EditorWindow*>(puglGetHandle(view))->handle(*event);
}

PuglStatus EditorWindow::handle(const PuglEvent& event) {
    ScopedImGuiContext context(mImGui);
    if (event.type == PUGL_KEY_PRESS || event.type == PUGL_BUTTON_PRESS) fprintf(stderr, "DEBUG %s key=%x state=%x nav=%s active=%x\n", event.type == PUGL_KEY_PRESS ? "key" : "button", event.type == PUGL_KEY_PRESS ? event.key.key : 0, event.key.state, ImGui::GetCurrentContext()->NavWindow ? ImGui::GetCurrentContext()->NavWindow->Name : "-", ImGui::GetCurrentContext()->ActiveId);
    if (event.type == PUGL_REALIZE)
        return startRenderer();
    if (event.type == PUGL_UNREALIZE)
        return stopRenderer();
    if (event.type == PUGL_EXPOSE)
        return drawFrame();
    if (event.type == PUGL_DATA_OFFER)
        return mClipboard.acceptOffer(event.offer);
    if (event.type == PUGL_DATA)
        return mClipboard.receive(event.data);
    if (mClipboard.requestPasteFor(event))
        return PUGL_SUCCESS;
    if (event.type == PUGL_BUTTON_PRESS)
        puglGrabFocus(mView);
    forwardToImGui(ImGui::GetIO(), event);
    return PUGL_SUCCESS;
}

PuglStatus EditorWindow::startRenderer() {
    return ImGui_ImplOpenGL3_Init(kGlslVersion) ? PUGL_SUCCESS : PUGL_BACKEND_FAILED;
}

PuglStatus EditorWindow::stopRenderer() {
    ImGui_ImplOpenGL3_Shutdown();
    return PUGL_SUCCESS;
}

PuglStatus EditorWindow::drawFrame() {
    const PuglArea size = puglGetSizeHint(mView, PUGL_CURRENT_SIZE);
    const double now = puglGetTime(mWorld);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(size.width, size.height);
    io.DeltaTime = static_cast<float>(std::max(now - mLastFrameTime, kMinFrameTime));
    mLastFrameTime = now;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    mDrawContents();
    ImGui::Render();

    glViewport(0, 0, size.width, size.height);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    updateCursor();
    return PUGL_SUCCESS;
}

void EditorWindow::updateCursor() {
    const PuglCursor cursor = puglCursorFor(ImGui::GetMouseCursor());
    if (cursor == mCursor)
        return;
    mCursor = cursor;
    puglSetCursor(mView, cursor);
}

}
