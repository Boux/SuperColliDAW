#include "SignatureHint.h"

#include <imgui.h>

#include <algorithm>
#include <optional>
#include <string>

namespace supercollidaw {

namespace {

constexpr size_t kRecentReplies = 32;
constexpr ImGuiWindowFlags kHintFlags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs
    | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings;

std::optional<size_t> namedParameter(const Signature& signature, const std::string& name) {
    const auto found = std::find_if(signature.parameters.begin(), signature.parameters.end(), [&name](const Parameter& parameter) { return parameter.name == name; });
    return found == signature.parameters.end() ? std::nullopt : std::optional<size_t>(found - signature.parameters.begin());
}

// Every argument from the ...varargs one on lands in it.
std::optional<size_t> activeParameter(const Signature& signature, const CallContext& call) {
    if (!call.keyword.empty())
        return namedParameter(signature, call.keyword);
    const auto variadic = std::find_if(signature.parameters.begin(), signature.parameters.end(), [](const Parameter& parameter) { return parameter.name.starts_with("..."); });
    const size_t variadicIndex = variadic - signature.parameters.begin();
    if (variadic != signature.parameters.end() && call.argument >= variadicIndex)
        return variadicIndex;
    return call.argument < signature.parameters.size() ? std::optional<size_t>(call.argument) : std::nullopt;
}

void drawText(const std::string& text, ImGuiCol color) {
    ImGui::SameLine(0.f, 0.f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(color));
    ImGui::TextUnformatted(text.c_str());
    ImGui::PopStyleColor();
}

std::string describe(const Parameter& parameter) { return parameter.defaultValue.empty() ? parameter.name : parameter.name + " = " + parameter.defaultValue; }

std::string describe(const Signature& signature) {
    std::string text = signature.label + "(";
    for (size_t index = 0; index < signature.parameters.size(); ++index)
        text += (index > 0 ? ", " : "") + describe(signature.parameters[index]);
    return text + ")";
}

std::string moreText(const SignatureHelp& help) {
    const size_t hidden = help.total - std::min(help.total, help.signatures.size());
    return hidden > 0 ? "and " + std::to_string(hidden) + " more" : std::string();
}

ImVec2 hintSize(const SignatureHelp& help, const std::string& more) {
    const ImGuiStyle& style = ImGui::GetStyle();
    float width = ImGui::CalcTextSize(more.c_str()).x;
    for (const Signature& signature : help.signatures)
        width = std::max(width, ImGui::CalcTextSize(describe(signature).c_str()).x);
    const size_t lines = help.signatures.size() + (more.empty() ? 0 : 1);
    const float height = static_cast<float>(lines) * ImGui::GetTextLineHeightWithSpacing() - style.ItemSpacing.y;
    return ImVec2(width + 2.f * style.WindowPadding.x, height + 2.f * style.WindowPadding.y);
}

// ImGui keeps windows inside the display only when it places them itself, so the hint moves left of the right edge, and under the line when there is no room above.
ImVec2 hintPosition(ImVec2 caret, float lineHeight, ImVec2 size) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float right = std::max(viewport->Pos.x, viewport->Pos.x + viewport->Size.x - size.x);
    const float above = caret.y - size.y;
    return ImVec2(std::clamp(caret.x, viewport->Pos.x, right), above >= viewport->Pos.y ? above : caret.y + lineHeight);
}

void drawSignature(const Signature& signature, std::optional<size_t> active) {
    ImGui::TextUnformatted((signature.label + "(").c_str());
    for (size_t index = 0; index < signature.parameters.size(); ++index) {
        if (index > 0)
            drawText(", ", ImGuiCol_Text);
        drawText(describe(signature.parameters[index]), index == active ? ImGuiCol_TextLink : ImGuiCol_Text);
    }
    drawText(")", ImGuiCol_Text);
}

}

void SignatureHint::update() {
    const std::optional<CallContext> call = mEditor.focused() ? enclosingCall(linesBeforeCursor(mEditor)) : std::nullopt;
    if (!call)
        return;
    request(call->callee);
    const SignatureHelp* help = recentReply(call->callee);
    if (help && !help->signatures.empty())
        draw(*help, *call);
}

void SignatureHint::show(const SignatureHelp& help) {
    mRecent.push_front(help);
    if (mRecent.size() > kRecentReplies)
        mRecent.pop_back();
}

void SignatureHint::request(const std::string& callee) {
    if (callee == mRequested)
        return;
    mRequested = callee;
    mRequest(callee);
}

const SignatureHelp* SignatureHint::recentReply(const std::string& callee) const {
    const auto found = std::find_if(mRecent.begin(), mRecent.end(), [&callee](const SignatureHelp& help) { return help.callee == callee; });
    return found == mRecent.end() ? nullptr : &*found;
}

void SignatureHint::draw(const SignatureHelp& help, const CallContext& call) const {
    const std::string more = moreText(help);
    ImGui::SetNextWindowPos(hintPosition(mEditor.screenPosition(mEditor.GetMainCursorPosition()), mEditor.GetLineHeight(), hintSize(help, more)));
    ImGui::Begin("##signature", nullptr, kHintFlags);
    for (const Signature& signature : help.signatures)
        drawSignature(signature, activeParameter(signature, call));
    if (!more.empty())
        ImGui::TextDisabled("%s", more.c_str());
    ImGui::End();
}

}
