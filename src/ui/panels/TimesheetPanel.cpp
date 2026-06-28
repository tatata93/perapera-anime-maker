// このファイルの役割:
// 正式タイムシートUIの表示専用スケルトンを描画する。
// Step 3では編集・保存・キャンバス反映は行わない。

#include "ui/panels/TimesheetPanel.h"

#include <algorithm>
#include <string>

#include <imgui.h>

namespace perapera::ui {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

int safeTotalFrames(const TimesheetPanelViewModel& data)
{
    return std::max(1, data.totalFrames);
}

std::string displayNameForColumn(const TimesheetPanelCellColumn& cell, int fallbackIndex)
{
    if (!cell.displayName.empty()) {
        return cell.displayName;
    }
    if (!cell.cellId.empty()) {
        return cell.cellId;
    }
    return std::string{"セル"} + std::to_string(fallbackIndex + 1);
}

void drawWindowPolicyNotice()
{
    ImGui::TextUnformatted(u8c(u8"Step 3: 表示専用スケルトン"));
    ImGui::TextWrapped(u8c(u8"この段階では編集・保存・キャンバス反映は行いません。縦型タイムシート表、T選択、追加欄、独立ウィンドウの土台だけを確認します。"));
    ImGui::TextWrapped(u8c(u8"大ウィンドウ外へのドラッグは、後続Stepで Dear ImGui multi-viewports を有効化できる場合に対応します。現段階では通常のImGuiウィンドウとして移動できます。"));
}

void drawPlaceholderCell(int frameIndex, int selectedFrame)
{
    if (frameIndex == selectedFrame) {
        ImGui::TextUnformatted(u8c(u8"□"));
        return;
    }
    ImGui::TextUnformatted(u8c(u8"—"));
}

} // namespace

TimesheetPanelResult drawTimesheetPanel(const TimesheetPanelViewModel& data, TimesheetPanelState& state)
{
    TimesheetPanelResult result;
    const bool wasOpen = state.windowOpen;
    const int totalFrames = safeTotalFrames(data);
    state.selectedTimelineFrame = std::clamp(state.selectedTimelineFrame, 0, totalFrames - 1);
    result.selectedTimelineFrame = state.selectedTimelineFrame;

    if (!state.showDetachedWindow || !state.windowOpen) {
        result.windowOpenChanged = wasOpen != state.windowOpen;
        return result;
    }

    ImGui::SetNextWindowSize(ImVec2(760.0f, 520.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8c(u8"タイムシート##FormalTimesheetPanel"), &state.windowOpen);

    result.windowOpenChanged = wasOpen != state.windowOpen;

    drawWindowPolicyNotice();
    ImGui::Separator();

    ImGui::Text(u8c(u8"現在T: %d / %d"), state.selectedTimelineFrame + 1, totalFrames);
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"先頭"))) {
        if (state.selectedTimelineFrame != 0) {
            state.selectedTimelineFrame = 0;
            result.timelineFrameChanged = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"前"))) {
        const int nextFrame = std::max(0, state.selectedTimelineFrame - 1);
        if (nextFrame != state.selectedTimelineFrame) {
            state.selectedTimelineFrame = nextFrame;
            result.timelineFrameChanged = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"次"))) {
        const int nextFrame = std::min(totalFrames - 1, state.selectedTimelineFrame + 1);
        if (nextFrame != state.selectedTimelineFrame) {
            state.selectedTimelineFrame = nextFrame;
            result.timelineFrameChanged = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"最後"))) {
        const int nextFrame = totalFrames - 1;
        if (nextFrame != state.selectedTimelineFrame) {
            state.selectedTimelineFrame = nextFrame;
            result.timelineFrameChanged = true;
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"追加欄"), &state.showInstructionColumns);

    ImGui::Text(u8c(u8"FPS: %d / セル列: %d"), data.fps, static_cast<int>(data.cells.size()));

    const int baseColumns = 1 + std::max(1, static_cast<int>(data.cells.size()));
    const int instructionColumns = state.showInstructionColumns ? 4 : 0;
    const int columnCount = baseColumns + instructionColumns;

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_ScrollX |
                                  ImGuiTableFlags_SizingFixedFit;

    if (ImGui::BeginTable("FormalTimesheetSkeletonTable", columnCount, flags, ImVec2(0.0f, 0.0f))) {
        ImGui::TableSetupColumn(u8c(u8"T"), ImGuiTableColumnFlags_WidthFixed, 48.0f);
        if (data.cells.empty()) {
            ImGui::TableSetupColumn(u8c(u8"セル"), ImGuiTableColumnFlags_WidthFixed, 96.0f);
        } else {
            for (int index = 0; index < static_cast<int>(data.cells.size()); ++index) {
                const std::string label = displayNameForColumn(data.cells[static_cast<std::size_t>(index)], index);
                ImGui::TableSetupColumn(label.c_str(), ImGuiTableColumnFlags_WidthFixed, 96.0f);
            }
        }
        if (state.showInstructionColumns) {
            ImGui::TableSetupColumn(u8c(u8"セリフ"), ImGuiTableColumnFlags_WidthFixed, 160.0f);
            ImGui::TableSetupColumn(u8c(u8"カメラ"), ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn(u8c(u8"撮影"), ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn(u8c(u8"素材メモ"), ImGuiTableColumnFlags_WidthFixed, 180.0f);
        }
        ImGui::TableSetupScrollFreeze(1, 1);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(totalFrames, state.rowHeight);
        while (clipper.Step()) {
            for (int frame = clipper.DisplayStart; frame < clipper.DisplayEnd; ++frame) {
                const bool selected = frame == state.selectedTimelineFrame;
                ImGui::TableNextRow(ImGuiTableRowFlags_None, state.rowHeight);

                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(frame);
                if (ImGui::Selectable(std::to_string(frame + 1).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    if (state.selectedTimelineFrame != frame) {
                        state.selectedTimelineFrame = frame;
                        result.timelineFrameChanged = true;
                    }
                }
                ImGui::PopID();

                const int cellColumnCount = std::max(1, static_cast<int>(data.cells.size()));
                for (int cellColumn = 0; cellColumn < cellColumnCount; ++cellColumn) {
                    ImGui::TableSetColumnIndex(1 + cellColumn);
                    drawPlaceholderCell(frame, state.selectedTimelineFrame);
                }

                if (state.showInstructionColumns) {
                    const int firstInstructionColumn = 1 + cellColumnCount;
                    ImGui::TableSetColumnIndex(firstInstructionColumn + 0);
                    ImGui::TextUnformatted(u8c(u8""));
                    ImGui::TableSetColumnIndex(firstInstructionColumn + 1);
                    ImGui::TextUnformatted(u8c(u8""));
                    ImGui::TableSetColumnIndex(firstInstructionColumn + 2);
                    ImGui::TextUnformatted(u8c(u8""));
                    ImGui::TableSetColumnIndex(firstInstructionColumn + 3);
                    ImGui::TextUnformatted(u8c(u8""));
                }
            }
        }
        ImGui::EndTable();
    }

    result.selectedTimelineFrame = state.selectedTimelineFrame;
    ImGui::End();
    return result;
}

} // namespace perapera::ui
