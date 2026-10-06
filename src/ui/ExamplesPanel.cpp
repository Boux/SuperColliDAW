#include "ExamplesPanel.h"

#include "Icons.h"
#include "SuperColliderLanguage.h"

namespace supercollidaw {

ExamplesPanel::ExamplesPanel(std::vector<Example> examples, const IconButtons& icons): mExamples(std::move(examples)), mIcons(icons) {
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
    drawCombo();
    drawCopyButton();
    mViewer.Render("example");
}

void ExamplesPanel::drawCombo() {
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (!ImGui::BeginCombo("##examples", mExamples[mSelected].title.c_str(), ImGuiComboFlags_HeightLarge))
        return;
    for (size_t index = 0; index < mExamples.size(); ++index)
        drawComboItem(index);
    ImGui::EndCombo();
}

void ExamplesPanel::drawComboItem(size_t index) {
    if (ImGui::Selectable(mExamples[index].title.c_str(), index == mSelected))
        select(index);
}

void ExamplesPanel::drawCopyButton() {
    if (mIcons.button(kIconCopy, "Copy the whole example"))
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
