// このファイルの役割:
// 簡易タイムラインの実装。
// Cell/FrameポインタをIDに使わず、固定IDとindexだけで操作ボタンを分離する。

#include "ui/panels/TimelinePanel.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

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

    ImGui::PushID("TimelinePanel_v26_drawing_frame_boxes_first");

    ImGui::TextUnformatted(u8c(u8"作画Fタイムライン"));
    ImGui::SameLine();
    ImGui::Text(u8c(u8"作画F数: %d"), static_cast<int>(cell.frames.size()));

    if (cell.frames.empty()) {
        ImGui::TextDisabled("no drawing frames");
        ImGui::PopID();
        return action;
    }

    activeFrameIndex = std::clamp(activeFrameIndex, 0, static_cast<int>(cell.frames.size()) - 1);

    ImGui::SameLine();
    ImGui::Text(u8c(u8"選択: 作画F%d"), activeFrameIndex + 1);

    int scrollCommand = 0;
    if (ImGui::SmallButton("|<##Timeline_First_v26")) {
        scrollCommand = -2;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("<##Timeline_Left_v26")) {
        scrollCommand = -1;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(">##Timeline_Right_v26")) {
        scrollCommand = 1;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(">|##Timeline_Last_v26")) {
        scrollCommand = 2;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"選択へ##Timeline_ScrollActive_v26"))) {
        scrollCommand = 3;
    }
    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"前オニオン##Timeline_OnionPrev_v26"), &onionPrevious);
    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"次オニオン##Timeline_OnionNext_v26"), &onionNext);

    // Timesheet Rebuild Step 7.3:
    // 表示順を「作画Fの四角列 → 操作ボタン」に固定する。
    // 3つの作画Fがあれば、3つの四角が必ず先に見える。
    const float frameButtonWidth = 58.0f;
    const float frameButtonHeight = 42.0f;
    const float frameButtonGap = std::max(5.0f, ImGui::GetStyle().ItemSpacing.x);
    const float leftPadding = 6.0f;
    const float topPadding = 6.0f;
    const float frameStep = frameButtonWidth + frameButtonGap;
    const int frameCount = static_cast<int>(cell.frames.size());
    const float contentWidth = leftPadding * 2.0f
        + static_cast<float>(frameCount) * frameStep
        + frameButtonGap;
    const float childHeight = frameButtonHeight + topPadding * 2.0f + ImGui::GetStyle().ScrollbarSize + 4.0f;

    ImGui::SetNextWindowContentSize(ImVec2(contentWidth, 0.0f));
    ImGui::BeginChild("TimelineDrawingFrameBoxes_v26",
                      ImVec2(0.0f, childHeight),
                      true,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysHorizontalScrollbar |
                          ImGuiWindowFlags_NoScrollWithMouse);

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

    const float visibleWidth = std::max(frameStep,
                                        ImGui::GetWindowWidth() -
                                            ImGui::GetStyle().WindowPadding.x * 2.0f -
                                            ImGui::GetStyle().ScrollbarSize);
    const float viewMinX = ImGui::GetScrollX();
    const float viewMaxX = viewMinX + visibleWidth;
    const int firstVisibleFrame = std::clamp(
        static_cast<int>(std::floor((viewMinX - leftPadding - frameButtonWidth) / frameStep)) - 2,
        0,
        frameCount);
    const int lastVisibleFrame = std::clamp(
        static_cast<int>(std::ceil((viewMaxX - leftPadding) / frameStep)) + 3,
        firstVisibleFrame,
        frameCount);

    for (int index = firstVisibleFrame; index < lastVisibleFrame; ++index) {
        ImGui::PushID(index);
        const float x = leftPadding + static_cast<float>(index) * frameStep;
        ImGui::SetCursorPos(ImVec2(x, topPadding));
        const bool selected = index == activeFrameIndex;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.47f, 0.86f, 1.0f));
        }
        const std::string label = std::string(u8c(u8"作F")) + std::to_string(index + 1) +
            "##Timeline_DrawingFrameBox_v26";
        if (ImGui::Button(label.c_str(), ImVec2(frameButtonWidth, frameButtonHeight))) {
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

    ImGui::TextDisabled(u8c(u8"用語: 作画F=描く絵の番号 / T=タイムシート位置 / コマ数=時間の長さ"));

    if (ImGui::SmallButton(u8c(u8"作画F追加##Timeline_AddFrame_v26"))) {
        action = TimelinePanelAction::AddFrame;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"作画F複製##Timeline_DuplicateFrame_v26"))) {
        action = TimelinePanelAction::DuplicateFrame;
    }
    ImGui::SameLine();

    const bool canDelete = cell.frames.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::SmallButton(u8c(u8"作画F削除##Timeline_DeleteFrame_v26"))) {
        action = TimelinePanelAction::DeleteFrame;
    }
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
