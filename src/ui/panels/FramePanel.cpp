// このファイルの役割:
// フレーム情報パネルの実装。
// Dear ImGuiのID衝突を避けるため、各ボタンをPushIDで完全に分離する。

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
    ImGui::PushID("FramePanel_v4");
    ImGui::PushID(static_cast<const void*>(&cell));

    ImGui::TextUnformatted(u8c(u8"フレーム"));
    ImGui::Text("count: %d", static_cast<int>(cell.frames.size()));

    FramePanelAction action = FramePanelAction::None;

    ImGui::PushID("AddFrameButton_v4");
    if (ImGui::Button(u8c(u8"フレーム追加"), ImVec2(-1.0f, 0.0f))) {
        action = FramePanelAction::AddFrame;
    }
    ImGui::PopID();

    ImGui::PushID("DuplicateFrameButton_v4");
    if (ImGui::Button(u8c(u8"フレーム複製"), ImVec2(-1.0f, 0.0f))) {
        action = FramePanelAction::DuplicateFrame;
    }
    ImGui::PopID();

    const bool canDelete = cell.frames.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    ImGui::PushID("DeleteFrameButton_v4");
    if (ImGui::Button(u8c(u8"フレーム削除"), ImVec2(-1.0f, 0.0f))) {
        action = FramePanelAction::DeleteFrame;
    }
    ImGui::PopID();
    if (!canDelete) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip(u8c(u8"最後の1フレームは削除できません。先に追加または複製してください。"));
        }
    }

    if (cell.frames.empty()) {
        ImGui::TextDisabled("no frames");
        ImGui::PopID();
        ImGui::PopID();
        return action;
    }

    activeFrameIndex = std::clamp(activeFrameIndex, 0, static_cast<int>(cell.frames.size()) - 1);
    Frame& frame = cell.frames[static_cast<std::size_t>(activeFrameIndex)];
    ImGui::Text("active: %d / %d", activeFrameIndex + 1, static_cast<int>(cell.frames.size()));
    ImGui::Text("name: %s", frame.name.c_str());
    ImGui::SetNextItemWidth(110.0f);
    ImGui::PushID("FrameDurationInput_v4");
    ImGui::InputInt(u8c(u8"尺"), &frame.durationFrames);
    ImGui::PopID();
    frame.durationFrames = std::max(1, frame.durationFrames);

    ImGui::PopID();
    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
