#include "EditorWindow.h"

#include "FontMetrics.h"
#include "Fonts.h"
#include "IconButtons.h"
#include "PuglImGuiInput.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <pugl/gl.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <string_view>
#include <utility>

namespace supercollidaw {

namespace {

constexpr char kGlslVersion[] = "#version 330 core";
constexpr double kMinFrameTime = 1.0 / 1000.0;
// Every outline font's em square gets this height at 100%, so switching fonts keeps the letters the same size; DejaVu Sans Mono comes out at 15 px.
constexpr float kEmSize = 13.f;

class ScopedImGuiContext {
public:
    explicit ScopedImGuiContext(ImGuiContext* context): mPrevious(ImGui::GetCurrentContext()) { ImGui::SetCurrentContext(context); }
    ~ScopedImGuiContext() { ImGui::SetCurrentContext(mPrevious); }

private:
    ImGuiContext* mPrevious;
};

bool isPixelFont(const BundledFont& font) { return font.pixelHeight > 0.f; }

// ImGui sizes a font by its ascent plus descent, which takes a different share of the em square in every font.
float baseSize(const BundledFont& font, const FontMetrics& metrics) {
    if (isPixelFont(font))
        return font.pixelHeight;
    return kEmSize * static_cast<float>(metrics.ascent - metrics.descent) / static_cast<float>(metrics.unitsPerEm);
}

// A pixel font is only sharp at whole multiples of its pixel height, so its size snaps after the DPI scale.
float textSize(const BundledFont& font, float baseSize, int sizePercent, double dpiScale) {
    const double ratio = sizePercent / 100.0;
    if (!isPixelFont(font))
        return static_cast<float>(baseSize * ratio);
    return static_cast<float>(baseSize * std::max(1.0, std::round(ratio * dpiScale)) / dpiScale);
}

// ImGui takes font data as void* but never writes to it.
void* fontData(std::span<const unsigned char> data) { return const_cast<unsigned char*>(data.data()); }

ImFont* addTextFont(ImGuiIO& io, const BundledFont& font, float size) {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.PixelSnapH = isPixelFont(font);
    return io.Fonts->AddFontFromMemoryTTF(fontData(font.data), static_cast<int>(font.data.size()), size, &config);
}

ImFont* addIconFont(ImGuiIO& io) {
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    return io.Fonts->AddFontFromMemoryTTF(fontData(iconFontData()), static_cast<int>(iconFontData().size()), kIconSize, &config);
}

}

EditorWindow::EditorWindow(PuglNativeView parent, uint32_t width, uint32_t height, double scale, DrawContents drawContents):
    mDrawContents(std::move(drawContents)),
    mWorld(puglNewWorld(PUGL_MODULE, 0)),
    mView(puglNewView(mWorld)),
    mClipboard(mView),
    mImGui(ImGui::CreateContext()),
    mScale(scale) {
    ScopedImGuiContext context(mImGui);
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().BackendPlatformName = "pugl";
    addFonts(ImGui::GetIO());
    mClipboard.install();

    puglSetWorldString(mWorld, PUGL_CLASS_NAME, "SuperColliDAW");
    useSystemKeyRepeat(ImGui::GetIO());
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
    mScale = scale;
    mStyleOutdated = true;
}

void EditorWindow::setSettings(const EditorSettings& settings) {
    if (settings == mSettings)
        return;
    mSettings = settings;
    mStyleOutdated = true;
}

double EditorWindow::systemScale() const { return puglGetScaleFactor(mView); }

void EditorWindow::show() { puglShow(mView, PUGL_SHOW_PASSIVE); }

void EditorWindow::hide() { puglHide(mView); }

void EditorWindow::idle() {
    puglObscureView(mView);
    processEvents();
}

void EditorWindow::processEvents() { puglUpdate(mWorld, 0.0); }

void EditorWindow::addFonts(ImGuiIO& io) {
    for (const BundledFont& font : bundledFonts()) {
        const std::optional<FontMetrics> metrics = readFontMetrics(font.data);
        if (!metrics)
            continue;
        const float size = baseSize(font, *metrics);
        mFonts.push_back({ &font, addTextFont(io, font, size), size });
    }
    mIconFont = addIconFont(io);
}

FixedFonts EditorWindow::fixedFonts() const {
    const LoadedFont& text = loadedFont(kDefaultFont);
    return { mIconFont, text.font, text.baseSize };
}

const EditorWindow::LoadedFont& EditorWindow::loadedFont(std::string_view name) const {
    const auto found = std::ranges::find(mFonts, name, [](const LoadedFont& font) { return std::string_view(font.source->name); });
    return found != mFonts.end() ? *found : mFonts.front();
}

void EditorWindow::applyStyle() {
    const LoadedFont& font = loadedFont(mSettings.font);
    ImGuiStyle style;
    ImGui::StyleColorsDark(&style);
    style.ScaleAllSizes(static_cast<float>(mScale));
    // The size setting goes into FontSizeBase, because ImGui applies FontScaleMain to the icon font too.
    style.FontSizeBase = textSize(*font.source, font.baseSize, mSettings.fontSizePercent, mScale);
    style.FontScaleDpi = static_cast<float>(mScale);
    ImGui::GetStyle() = style;
    ImGui::GetIO().FontDefault = font.font;
}

PuglStatus EditorWindow::onEvent(PuglView* view, const PuglEvent* event) {
    return static_cast<EditorWindow*>(puglGetHandle(view))->handle(*event);
}

PuglStatus EditorWindow::handle(const PuglEvent& event) {
    ScopedImGuiContext context(mImGui);
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
    mRendererStarted = ImGui_ImplOpenGL3_Init(kGlslVersion);
    return mRendererStarted ? PUGL_SUCCESS : PUGL_BACKEND_FAILED;
}

// pugl sends PUGL_UNREALIZE even when PUGL_REALIZE failed, and ImGui keeps no renderer after a failed init.
PuglStatus EditorWindow::stopRenderer() {
    if (std::exchange(mRendererStarted, false))
        ImGui_ImplOpenGL3_Shutdown();
    return PUGL_SUCCESS;
}

PuglStatus EditorWindow::drawFrame() {
    if (std::exchange(mStyleOutdated, false))
        applyStyle();
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
