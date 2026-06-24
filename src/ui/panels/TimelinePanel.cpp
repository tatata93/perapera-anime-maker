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

    ImGui::PushID("TimelinePanel_v24_real_scroll");

    ImGui::TextUnformatted(u8c(u8"タイムライン"));
    ImGui::SameLine();
    ImGui::Text("frames: %d", static_cast<int>(cell.frames.size()));
    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"前オニオン##Timeline_OnionPrev_v24"), &onionPrevious);
    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"次オニオン##Timeline_OnionNext_v24"), &onionNext);

    if (ImGui::SmallButton(u8c(u8"TL フレーム追加##Timeline_AddFrame_v24"))) {
        action = TimelinePanelAction::AddFrame;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"TL フレーム複製##Timeline_DuplicateFrame_v24"))) {
        action = TimelinePanelAction::DuplicateFrame;
    }
    ImGui::SameLine();

    const bool canDelete = cell.frames.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::SmallButton(u8c(u8"TL フレーム削除##Timeline_DeleteFrame_v24"))) {
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

    ImGui::Separator();
    ImGui::TextDisabled(u8c(u8"フレーム列: ホイール / 下のバー / << < > >> で横スクロール"));

    int scrollCommand = 0;
    if (ImGui::SmallButton("<<##Timeline_First_v24")) {
        scrollCommand = -2;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("<##Timeline_Left_v24")) {
        scrollCommand = -1;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(">##Timeline_Right_v24")) {
        scrollCommand = 1;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(">>##Timeline_Last_v24")) {
        scrollCommand = 2;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"選択へ##Timeline_ScrollActive_v24"))) {
        scrollCommand = 3;
    }

    const float frameButtonWidth = 82.0f;
    const float frameButtonHeight = 32.0f;
    const float frameButtonGap = ImGui::GetStyle().ItemSpacing.x;
    const float leftPadding = 6.0f;
    const float topPadding = 6.0f;
    const float frameStep = frameButtonWidth + frameButtonGap;
    const float contentWidth = leftPadding * 2.0f
        + static_cast<float>(cell.frames.size()) * frameStep
        + frameButtonGap;
    const float childHeight = frameButtonHeight + topPadding * 2.0f + ImGui::GetFrameHeightWithSpacing();

    ImGui::SetNextWindowContentSize(ImVec2(contentWidth, 0.0f));
    ImGui::BeginChild("TimelineFrameStripScroll_v24_real",
                      ImVec2(0.0f, childHeight),
                      true,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysHorizontalScrollbar |
                          ImGuiWindowFlags_NoMove);

    const float scrollMax = ImGui::GetScrollMaxX();
    const float pageScroll = std::max(frameStep, ImGui::GetWindowContentRegionMax().x - frameButtonWidth);
    if (scrollCommand == -2) {
        ImGui::SetScrollX(0.0f);
    } else if (scrollCommand == -1) {
        ImGui::SetScrollX(std::max(0.0f, ImGui::GetScrollX() - pageScroll));
    } else if (scrollCommand == 1) {
        ImGui::SetScrollX(std::min(scrollMax, ImGui::GetScrollX() + pageScroll));
    } else if (scrollCommand == 2) {
        ImGui::SetScrollX(scrollMax);
    } else if (scrollCommand == 3) {
        const float target = leftPadding + static_cast<float>(activeFrameIndex) * frameStep;
        ImGui::SetScrollX(std::clamp(target - frameButtonGap, 0.0f, scrollMax));
    }

    const ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f) {
        ImGui::SetScrollX(std::clamp(ImGui::GetScrollX() - io.MouseWheel * frameStep * 2.0f,
                                     0.0f,
                                     scrollMax));
    }

    for (int index = 0; index < static_cast<int>(cell.frames.size()); ++index) {
        ImGui::PushID(index);
        const float x = leftPadding + static_cast<float>(index) * frameStep;
        ImGui::SetCursorPos(ImVec2(x, topPadding));
        const bool selected = index == activeFrameIndex;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.47f, 0.86f, 1.0f));
        }
        const Frame& frame = cell.frames[static_cast<std::size_t>(index)];
        if (ImGui::Button(frame.name.c_str(), ImVec2(frameButtonWidth, frameButtonHeight))) {
            activeFrameIndex = index;
        }
        if (selected) {
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
    }

    ImGui::SetCursorPos(ImVec2(contentWidth, topPadding + frameButtonHeight));
    ImGui::Dummy(ImVec2(1.0f, 1.0f));
    ImGui::EndChild();

    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
