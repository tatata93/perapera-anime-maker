// このファイルの役割:
// 簡易タイムラインの実装。
// Cell/FrameポインタをIDに使わず、固定IDとindexだけで操作ボタンを分離する。

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

    ImGui::PushID("TimelinePanel_v10_noptr");

    ImGui::TextUnformatted(u8c(u8"タイムライン"));
    ImGui::SameLine();
    ImGui::Text("frames: %d", static_cast<int>(cell.frames.size()));
    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"前オニオン##Timeline_OnionPrev_v10"), &onionPrevious);
    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"次オニオン##Timeline_OnionNext_v10"), &onionNext);

    if (ImGui::SmallButton(u8c(u8"TL フレーム追加##Timeline_AddFrame_v10"))) {
        action = TimelinePanelAction::AddFrame;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"TL フレーム複製##Timeline_DuplicateFrame_v10"))) {
        action = TimelinePanelAction::DuplicateFrame;
    }
    ImGui::SameLine();

    const bool canDelete = cell.frames.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::SmallButton(u8c(u8"TL フレーム削除##Timeline_DeleteFrame_v10"))) {
        action = TimelinePanelAction::DeleteFrame;
    }
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    if (cell.frames.empty()) {
        ImGui::TextDisabled("no frames");
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
    return action;
}

} // namespace perapera::ui
