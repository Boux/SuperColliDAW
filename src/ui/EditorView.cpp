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
constexpr float kInitialExamplesShare = 0.5f;
constexpr float kMinPaneHeight = 60.f;
constexpr float kMinPaneWidth = 200.f;
constexpr float kSplitterThickness = 6.f;

TextEditor::DocSelection wholeLines(LineRange lines) { return { TextEditor::DocPos(lines.first, 0), TextEditor::DocPos(lines.last, kEndOfLine) }; }

TextEditor::DocSelection selectionOrRegion(const TextEditor& editor) {
    if (editor.MainCursorHasSelection())
        return editor.GetMainCursorSelection();
    return wholeLines(regionAround(linesOf(editor), editor.GetMainCursorPosition().line));
}

TextEditor::DocSelection selectionOrLine(const TextEditor& editor) {
    if (editor.MainCursorHasSelection())
        return editor.GetMainCursorSelection();
    const size_t line = editor.GetMainCursorPosition().line;
    return wholeLines({ line, line });
}

LineRange linesIn(const TextEditor::DocSelection& section) {
    const bool endsAtLineStart = section.end.index == 0 && section.end.line > section.start.line;
    return { section.start.line, section.end.line - (endsAtLineStart ? 1 : 0) };
}

ImVec2 splitterDrag(const char* id, const ImVec2& size, ImGuiMouseCursor cursor) {
    ImGui::InvisibleButton(id, size);
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(cursor);
    return ImGui::IsItemActive() ? ImGui::GetIO().MouseDelta : ImVec2(0.f, 0.f);
}

}

EditorView::EditorView(EditorActions actions, const PostLog& postLog, std::vector<Example> examples):
    mActions(std::move(actions)),
    mCompletion(mEditor, mActions.complete),
    mSignatureHint(mEditor, mActions.lookUpSignatures),
    mPostWindow(postLog),
    mExamples(std::move(examples)),
    mPostHeight(kInitialPostHeight),
    mExamplesShare(kInitialExamplesShare) {
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
    mFlash.update();
    drawToolbar();
    const float width = ImGui::GetContentRegionAvail().x - kSplitterThickness;
    ImGui::BeginChild("code and post", ImVec2(mShowExamples ? width - examplesWidth(width) : 0.f, 0.f));
    drawCodeAndPost();
    ImGui::EndChild();
    if (mShowExamples)
        drawExamples(width);
    ImGui::End();
}

void EditorView::drawCodeAndPost() {
    const float available = ImGui::GetContentRegionAvail().y - kSplitterThickness - 2.f * ImGui::GetStyle().ItemSpacing.y;
    mPostHeight = std::clamp(mPostHeight, kMinPaneHeight, std::max(kMinPaneHeight, available - kMinPaneHeight));
    mEditor.draw("code", ImVec2(0.f, available - mPostHeight));
    mCompletion.update();
    mSignatureHint.update();
    mPostHeight -= splitterDrag("post splitter", ImVec2(-1.f, kSplitterThickness), ImGuiMouseCursor_ResizeNS).y;
    mPostWindow.draw(ImVec2(0.f, 0.f));
}

void EditorView::drawExamples(float width) {
    ImGui::SameLine(0.f, 0.f);
    const float dragged = splitterDrag("examples splitter", ImVec2(kSplitterThickness, -1.f), ImGuiMouseCursor_ResizeEW).x;
    mExamplesShare = (examplesWidth(width) - dragged) / width;
    ImGui::SameLine(0.f, 0.f);
    mExamples.draw(ImVec2(0.f, 0.f));
}

float EditorView::examplesWidth(float width) const { return std::clamp(mExamplesShare * width, kMinPaneWidth, std::max(kMinPaneWidth, width - kMinPaneWidth)); }

void EditorView::drawToolbar() {
    if (ImGui::Button("Run all"))
        runAll();
    ImGui::SetItemTooltip("Stop everything and run the whole code.\nCtrl+Enter evaluates the block or selection, Shift+Enter the line.");
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
        mActions.stop();
    ImGui::SetItemTooltip("Stop all sound (Ctrl+.)");
    ImGui::SameLine();
    if (ImGui::Button("Reboot interpreter"))
        mActions.rebootInterpreter();
    ImGui::SetItemTooltip("Restart sclang and the server, then run the whole code (Ctrl+Shift+L)");
    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x * 4.f);
    drawFileButtons();
    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x * 4.f);
    drawExamplesButton();
    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x * 4.f);
    drawStatus();
}

void EditorView::drawFileButtons() {
    if (ImGui::Button("Open..."))
        mActions.open();
    ImGui::SetItemTooltip("Link this instance to a .scd file (Ctrl+O)");
    ImGui::SameLine();
    if (ImGui::Button("Save"))
        mActions.save(mEditor.GetText());
    ImGui::SetItemTooltip("Save the linked file (Ctrl+S)");
    ImGui::SameLine();
    if (ImGui::Button("Save as..."))
        mActions.saveAs(mEditor.GetText());
    ImGui::SetItemTooltip("Save to a new .scd file and link to it (Ctrl+Shift+S)");
    if (!mStatus.linked)
        return;
    ImGui::SameLine();
    if (ImGui::Button("Unlink"))
        mActions.unlink(mEditor.GetText());
    ImGui::SetItemTooltip("Keep the code in the project and stop following the file");
}

void EditorView::drawExamplesButton() {
    if (ImGui::Button(mShowExamples ? "Hide examples" : "Examples"))
        mShowExamples = !mShowExamples;
    ImGui::SetItemTooltip("Example code to run or copy, one per feature (F1)");
}

void EditorView::drawStatus() {
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("%s%s", mStatus.source.c_str(), mStatus.dirty ? " (modified)" : "");
}

void EditorView::handleShortcuts() {
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Enter, kOverEditor))
        evaluate(focusedEditor(), selectionOrRegion(focusedEditor()));
    if (ImGui::Shortcut(ImGuiMod_Shift | ImGuiKey_Enter, kOverEditor))
        evaluate(focusedEditor(), selectionOrLine(focusedEditor()));
    if (ImGui::Shortcut(ImGuiKey_F1, kOverEditor))
        mShowExamples = !mShowExamples;
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Period, kOverEditor))
        mActions.stop();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_L, kOverEditor))
        mActions.rebootInterpreter();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, kOverEditor))
        mActions.save(mEditor.GetText());
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, kOverEditor))
        mActions.saveAs(mEditor.GetText());
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, kOverEditor))
        mActions.open();
}

void EditorView::runAll() {
    mFlash.start(mEditor, { 0, mEditor.GetLineCount() - 1 });
    mActions.runAll(mEditor.GetText());
}

void EditorView::evaluate(TextEditor& editor, const TextEditor::DocSelection& section) {
    mFlash.start(editor, linesIn(section));
    mActions.evaluate(editor.GetSectionText(section));
}

TextEditor& EditorView::focusedEditor() { return mShowExamples && mExamples.focused() ? mExamples.viewer() : mEditor; }

}
