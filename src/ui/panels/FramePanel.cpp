// このファイルの役割:
// フレーム情報パネルの実装。
// Dear ImGuiのID衝突を避けるため、固定IDだけでスコープ分離する。

#include "ui/panels/FramePanel.h"

#include <algorithm>
#include <cstddef>

#include <imgui.h>

namespace perapera::ui {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

} // namespace

FramePanelAction drawFramePanel(Cell& cell, int& activeFrameIndex)
{
    ImGui::PushID("FramePanel_v10_noptr");

    ImGui::TextUnformatted(u8c(u8"フレーム"));
    ImGui::Text("count: %d", static_cast<int>(cell.frames.size()));

    FramePanelAction action = FramePanelAction::None;

    if (ImGui::Button(u8c(u8"追加##FramePanel_AddFrame_v10"), ImVec2(-1.0f, 0.0f))) {
        action = FramePanelAction::AddFrame;
    }
    if (ImGui::Button(u8c(u8"複製##FramePanel_DuplicateFrame_v10"), ImVec2(-1.0f, 0.0f))) {
        action = FramePanelAction::DuplicateFrame;
    }

    const bool canDelete = cell.frames.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(u8c(u8"削除##FramePanel_DeleteFrame_v10"), ImVec2(-1.0f, 0.0f))) {
        action = FramePanelAction::DeleteFrame;
    }
    if (!canDelete) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip(u8c(u8"最後の1フレームは削除できません。"));
        }
    }

    if (cell.frames.empty()) {
        ImGui::TextDisabled("no frames");
        ImGui::PopID();
        return action;
    }

    activeFrameIndex = std::clamp(activeFrameIndex, 0, static_cast<int>(cell.frames.size()) - 1);
    Frame& frame = cell.frames[static_cast<std::size_t>(activeFrameIndex)];
    ImGui::Text("active: %d / %d", activeFrameIndex + 1, static_cast<int>(cell.frames.size()));
    ImGui::Text("name: %s", frame.name.c_str());
    ImGui::SetNextItemWidth(110.0f);
    ImGui::InputInt(u8c(u8"尺##FramePanel_Duration_v10"), &frame.durationFrames);
    frame.durationFrames = std::max(1, frame.durationFrames);

    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
