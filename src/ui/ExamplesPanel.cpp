#include "ExamplesPanel.h"

#include "IconButton.h"
#include "Icons.h"
#include "SuperColliderLanguage.h"

namespace supercollidaw {

ExamplesPanel::ExamplesPanel(std::vector<Example> examples): mExamples(std::move(examples)) {
    mViewer.SetLanguage(superColliderLanguage());
    mViewer.SetTabSize(4);
    mViewer.SetReadOnlyEnabled(true);
    mViewer.SetShowMatchingBrackets(true);
    mViewer.SetShowWhitespacesEnabled(false);
    if (!mExamples.empty())
        select(0);
}

void ExamplesPanel::draw(const ImVec2& size) {
    ImGui::BeginChild("examples", size);
    drawContents();
    mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    ImGui::EndChild();
}

void ExamplesPanel::drawContents() {
    if (mExamples.empty())
        return ImGui::TextDisabled("No examples were found next to the plugin.");
    drawList();
    drawCopyButton();
    mViewer.Render("example");
}

void ExamplesPanel::drawList() {
    const float height = ImGui::GetTextLineHeightWithSpacing() * static_cast<float>(mExamples.size()) + ImGui::GetStyle().FramePadding.y * 2.f;
    if (!ImGui::BeginListBox("##examples", ImVec2(-FLT_MIN, height)))
        return;
    for (size_t index = 0; index < mExamples.size(); ++index)
        drawListItem(index);
    ImGui::EndListBox();
}

void ExamplesPanel::drawListItem(size_t index) {
    if (ImGui::Selectable(mExamples[index].title.c_str(), index == mSelected))
        select(index);
}

void ExamplesPanel::drawCopyButton() {
    if (iconButton(kIconCopy, "Copy the whole example"))
        ImGui::SetClipboardText(mExamples[mSelected].code.c_str());
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("Ctrl+Enter runs the block under the cursor");
}

void ExamplesPanel::select(size_t index) {
    mSelected = index;
    mViewer.SetText(mExamples[index].code);
}

}
