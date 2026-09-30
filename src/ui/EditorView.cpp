#include "EditorView.h"

#include "CodeRegion.h"
#include "SuperColliderLanguage.h"

#include <imgui.h>

#include <algorithm>
#include <limits>
#include <vector>

namespace supercollidaw {

namespace {

constexpr ImGuiWindowFlags kFullWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
constexpr ImGuiInputFlags kOverEditor = ImGuiInputFlags_RouteGlobal | ImGuiInputFlags_RouteOverFocused;
constexpr int kCodeChangedDelayMs = 200;
constexpr size_t kEndOfLine = std::numeric_limits<size_t>::max();
constexpr float kInitialPostHeight = 160.f;
constexpr float kMinPaneHeight = 60.f;
constexpr float kSplitterThickness = 6.f;

std::vector<std::string> linesOf(const TextEditor& editor) {
    std::vector<std::string> lines(editor.GetLineCount());
    for (size_t line = 0; line < lines.size(); ++line)
        lines[line] = editor.GetLineText(line);
    return lines;
}

}

EditorView::EditorView(EditorActions actions, const PostLog& postLog):
    mActions(std::move(actions)), mPostWindow(postLog), mPostHeight(kInitialPostHeight) {
    mEditor.SetLanguage(superColliderLanguage());
    mEditor.SetTabSize(4);
    mEditor.SetShowMatchingBrackets(true);
    mEditor.SetShowWhitespacesEnabled(false);
    mEditor.SetChangeCallback([this] { mActions.codeChanged(mEditor.GetText()); }, kCodeChangedDelayMs);
}

void EditorView::setCode(const std::string& code) { mEditor.SetText(code); }

void EditorView::draw() {
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("SuperColliDAW", nullptr, kFullWindowFlags);
    handleShortcuts();
    drawToolbar();
    const float available = ImGui::GetContentRegionAvail().y - kSplitterThickness - 2.f * ImGui::GetStyle().ItemSpacing.y;
    mPostHeight = std::clamp(mPostHeight, kMinPaneHeight, std::max(kMinPaneHeight, available - kMinPaneHeight));
    mEditor.Render("code", ImVec2(0.f, available - mPostHeight));
    drawSplitter();
    mPostWindow.draw(ImVec2(0.f, 0.f));
    ImGui::End();
}

void EditorView::drawSplitter() {
    ImGui::InvisibleButton("splitter", ImVec2(-1.f, kSplitterThickness));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive())
        mPostHeight -= ImGui::GetIO().MouseDelta.y;
}

void EditorView::drawToolbar() {
    if (ImGui::Button("Run all"))
        mActions.runAll(mEditor.GetText());
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
        mActions.stop();
    ImGui::SameLine();
    ImGui::TextDisabled("Ctrl+Enter evaluate block   Shift+Enter evaluate line   Ctrl+. stop");
}

void EditorView::handleShortcuts() {
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Enter, kOverEditor))
        mActions.evaluate(selectionOrRegion());
    if (ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_Enter, kOverEditor))
        mActions.evaluate(selectionOrLine());
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Period, kOverEditor))
        mActions.stop();
}

std::string EditorView::selectionOrRegion() const {
    if (mEditor.MainCursorHasSelection())
        return mEditor.GetSectionText(mEditor.GetMainCursorSelection());
    const LineRange region = regionAround(linesOf(mEditor), mEditor.GetMainCursorPosition().line);
    return mEditor.GetSectionText(TextEditor::DocPos(region.first, 0), TextEditor::DocPos(region.last, kEndOfLine));
}

std::string EditorView::selectionOrLine() const {
    if (mEditor.MainCursorHasSelection())
        return mEditor.GetSectionText(mEditor.GetMainCursorSelection());
    return mEditor.GetLineText(mEditor.GetMainCursorPosition().line);
}

}
