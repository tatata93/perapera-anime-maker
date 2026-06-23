// このファイルの役割:
// 簡易タイムラインの実装。
// フレーム操作ボタンのID衝突を避け、右サイドバーが使いにくい場合も下部から操作できるようにする。

#include "ui/panels/TimelinePanel.h"

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

TimelinePanelAction drawTimelinePanel(Cell& cell,
                                      int& activeFrameIndex,
                                      bool& onionPrevious,
                                      bool& onionNext)
{
    TimelinePanelAction action = TimelinePanelAction::None;

    ImGui::PushID("TimelinePanel_v4");
    ImGui::PushID(static_cast<const void*>(&cell));

    ImGui::TextUnformatted(u8c(u8"タイムライン"));
    ImGui::SameLine();
    ImGui::Text("frames: %d", static_cast<int>(cell.frames.size()));
    ImGui::SameLine();
    ImGui::PushID("OnionPreviousCheck_v4");
    ImGui::Checkbox(u8c(u8"前オニオン"), &onionPrevious);
    ImGui::PopID();
    ImGui::SameLine();
    ImGui::PushID("OnionNextCheck_v4");
    ImGui::Checkbox(u8c(u8"次オニオン"), &onionNext);
    ImGui::PopID();

    ImGui::PushID("TimelineAddFrameButton_v4");
    if (ImGui::SmallButton(u8c(u8"TL フレーム追加"))) {
        action = TimelinePanelAction::AddFrame;
    }
    ImGui::PopID();
    ImGui::SameLine();

    ImGui::PushID("TimelineDuplicateFrameButton_v4");
    if (ImGui::SmallButton(u8c(u8"TL フレーム複製"))) {
        action = TimelinePanelAction::DuplicateFrame;
    }
    ImGui::PopID();
    ImGui::SameLine();

    const bool canDelete = cell.frames.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    ImGui::PushID("TimelineDeleteFrameButton_v4");
    if (ImGui::SmallButton(u8c(u8"TL フレーム削除"))) {
        action = TimelinePanelAction::DeleteFrame;
    }
    ImGui::PopID();
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    if (cell.frames.empty()) {
        ImGui::TextDisabled("no frames");
        ImGui::PopID();
        ImGui::PopID();
        return action;
    }

    activeFrameIndex = std::clamp(activeFrameIndex, 0, static_cast<int>(cell.frames.size()) - 1);
    for (int index = 0; index < static_cast<int>(cell.frames.size()); ++index) {
        if (index > 0) {
            ImGui::SameLine();
        }
        ImGui::PushID(index);
        const bool selected = index == activeFrameIndex;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.47f, 0.86f, 1.0f));
        }
        const Frame& frame = cell.frames[static_cast<std::size_t>(index)];
        if (ImGui::Button(frame.name.c_str(), ImVec2(82.0f, 32.0f))) {
            activeFrameIndex = index;
        }
        if (selected) {
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
    }

    ImGui::PopID();
    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
