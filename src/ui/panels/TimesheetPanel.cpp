// このファイルの役割:
// 正式タイムシートUIの最小編集スケルトンを描画する。
// Step 4では数字、ホールド、空セル、原画、中割の入力をUI状態内の一時データとして扱う。

#include "ui/panels/TimesheetPanel.h"

#include <algorithm>
#include <string>
#include <utility>

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

bool kindUsesDrawingNumber(TimesheetPanelEntryKind kind)
{
    return kind == TimesheetPanelEntryKind::Drawing ||
        kind == TimesheetPanelEntryKind::Key ||
        kind == TimesheetPanelEntryKind::Inbetween;
}

const char* entryKindLabel(TimesheetPanelEntryKind kind)
{
    switch (kind) {
    case TimesheetPanelEntryKind::Empty:
        return u8c(u8"未記入");
    case TimesheetPanelEntryKind::Hold:
        return u8c(u8"ホールド");
    case TimesheetPanelEntryKind::Drawing:
        return u8c(u8"作画");
    case TimesheetPanelEntryKind::Null:
        return u8c(u8"空セル");
    case TimesheetPanelEntryKind::Key:
        return u8c(u8"原画");
    case TimesheetPanelEntryKind::Inbetween:
        return u8c(u8"中割");
    }
    return u8c(u8"未記入");
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

std::string cellIdForColumn(const TimesheetPanelViewModel& data, int cellColumn)
{
    if (cellColumn < 0 || cellColumn >= static_cast<int>(data.cells.size())) {
        return {};
    }
    const TimesheetPanelCellColumn& cell = data.cells[static_cast<std::size_t>(cellColumn)];
    if (!cell.cellId.empty()) {
        return cell.cellId;
    }
    return std::string{"__timesheet_panel_cell_"} + std::to_string(cellColumn);
}

TimesheetPanelEditableEntry* findEntry(
    TimesheetPanelState& state,
    int timelineFrame,
    const std::string& cellId)
{
    const auto it = std::find_if(
        state.entries.begin(),
        state.entries.end(),
        [&](const TimesheetPanelEditableEntry& entry) {
            return entry.timelineFrame == timelineFrame && entry.cellId == cellId;
        });
    return it == state.entries.end() ? nullptr : &(*it);
}

const TimesheetPanelEditableEntry* findEntry(
    const TimesheetPanelState& state,
    int timelineFrame,
    const std::string& cellId)
{
    const auto it = std::find_if(
        state.entries.begin(),
        state.entries.end(),
        [&](const TimesheetPanelEditableEntry& entry) {
            return entry.timelineFrame == timelineFrame && entry.cellId == cellId;
        });
    return it == state.entries.end() ? nullptr : &(*it);
}

void eraseEntry(TimesheetPanelState& state, int timelineFrame, const std::string& cellId)
{
    state.entries.erase(
        std::remove_if(
            state.entries.begin(),
            state.entries.end(),
            [&](const TimesheetPanelEditableEntry& entry) {
                return entry.timelineFrame == timelineFrame && entry.cellId == cellId;
            }),
        state.entries.end());
}

void setEntry(
    TimesheetPanelState& state,
    int timelineFrame,
    const std::string& cellId,
    TimesheetPanelEntryKind kind,
    int drawingFrameNumber)
{
    if (cellId.empty()) {
        return;
    }

    if (kind == TimesheetPanelEntryKind::Empty) {
        eraseEntry(state, timelineFrame, cellId);
        return;
    }

    TimesheetPanelEditableEntry* entry = findEntry(state, timelineFrame, cellId);
    if (entry == nullptr) {
        TimesheetPanelEditableEntry newEntry;
        newEntry.timelineFrame = timelineFrame;
        newEntry.cellId = cellId;
        state.entries.push_back(std::move(newEntry));
        entry = &state.entries.back();
    }

    entry->kind = kind;
    entry->drawingFrameNumber = kindUsesDrawingNumber(kind) ? std::max(1, drawingFrameNumber) : 0;
}

std::string displayLabelForEntry(const TimesheetPanelEditableEntry* entry, bool selected)
{
    if (entry == nullptr || entry->kind == TimesheetPanelEntryKind::Empty) {
        return selected ? u8c(u8"□") : u8c(u8"—");
    }

    switch (entry->kind) {
    case TimesheetPanelEntryKind::Hold:
        return u8c(u8"｜");
    case TimesheetPanelEntryKind::Null:
        return u8c(u8"×");
    case TimesheetPanelEntryKind::Drawing:
        return std::to_string(std::max(1, entry->drawingFrameNumber));
    case TimesheetPanelEntryKind::Key:
        return std::string{u8c(u8"原")} + std::to_string(std::max(1, entry->drawingFrameNumber));
    case TimesheetPanelEntryKind::Inbetween:
        return std::string{u8c(u8"中")} + std::to_string(std::max(1, entry->drawingFrameNumber));
    case TimesheetPanelEntryKind::Empty:
        break;
    }
    return selected ? u8c(u8"□") : u8c(u8"—");
}

void drawWindowPolicyNotice()
{
    ImGui::TextUnformatted(u8c(u8"Step 4: 最小編集スケルトン"));
    ImGui::TextWrapped(u8c(u8"この段階では、マスの編集結果はタイムシートウィンドウ内の一時状態だけに保持します。保存、キャンバス表示、再生、出力にはまだ反映しません。"));
    ImGui::TextWrapped(u8c(u8"セル列のマスを選択してから、作画F番号と種類ボタンで入力します。"));
}

void normalizeSelection(const TimesheetPanelViewModel& data, TimesheetPanelState& state)
{
    const int totalFrames = safeTotalFrames(data);
    state.selectedTimelineFrame = std::clamp(state.selectedTimelineFrame, 0, totalFrames - 1);

    if (data.cells.empty()) {
        state.selectedCellColumn = 0;
    } else {
        state.selectedCellColumn = std::clamp(state.selectedCellColumn, 0, static_cast<int>(data.cells.size()) - 1);
    }

    state.editDrawingFrameNumber = std::max(1, state.editDrawingFrameNumber);
}

void drawSelectedCellEditor(
    const TimesheetPanelViewModel& data,
    TimesheetPanelState& state,
    TimesheetPanelResult& result)
{
    if (data.cells.empty()) {
        ImGui::TextUnformatted(u8c(u8"セル列がありません。セルを追加するとタイムシート列が表示されます。"));
        return;
    }

    const int cellColumn = std::clamp(state.selectedCellColumn, 0, static_cast<int>(data.cells.size()) - 1);
    const std::string cellId = cellIdForColumn(data, cellColumn);
    const std::string cellName = displayNameForColumn(data.cells[static_cast<std::size_t>(cellColumn)], cellColumn);
    const TimesheetPanelEditableEntry* entry = findEntry(state, state.selectedTimelineFrame, cellId);

    ImGui::Text(
        u8c(u8"選択中: T%d / %s"),
        state.selectedTimelineFrame + 1,
        cellName.c_str());
    ImGui::SameLine();
    ImGui::Text(u8c(u8"現在: %s"), entryKindLabel(entry == nullptr ? TimesheetPanelEntryKind::Empty : entry->kind));

    ImGui::SetNextItemWidth(96.0f);
    if (ImGui::InputInt(u8c(u8"作画F番号"), &state.editDrawingFrameNumber)) {
        state.editDrawingFrameNumber = std::max(1, state.editDrawingFrameNumber);
    }

    auto apply = [&](TimesheetPanelEntryKind kind) {
        setEntry(state, state.selectedTimelineFrame, cellId, kind, state.editDrawingFrameNumber);
        result.entryChanged = true;
    };

    if (ImGui::SmallButton(u8c(u8"作画"))) {
        apply(TimesheetPanelEntryKind::Drawing);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"原画"))) {
        apply(TimesheetPanelEntryKind::Key);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"中割"))) {
        apply(TimesheetPanelEntryKind::Inbetween);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"ホールド"))) {
        apply(TimesheetPanelEntryKind::Hold);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"空セル"))) {
        apply(TimesheetPanelEntryKind::Null);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(u8c(u8"消去"))) {
        apply(TimesheetPanelEntryKind::Empty);
    }

    ImGui::TextWrapped(u8c(u8"表示記号: 数字=作画 / 原+数字=原画 / 中+数字=中割 / ｜=ホールド / ×=空セル / —=未記入"));
}

} // namespace

TimesheetPanelResult drawTimesheetPanel(const TimesheetPanelViewModel& data, TimesheetPanelState& state)
{
    TimesheetPanelResult result;
    const bool wasOpen = state.windowOpen;
    const int totalFrames = safeTotalFrames(data);
    normalizeSelection(data, state);
    result.selectedTimelineFrame = state.selectedTimelineFrame;
    result.selectedCellColumn = state.selectedCellColumn;

    if (!state.showDetachedWindow || !state.windowOpen) {
        result.windowOpenChanged = wasOpen != state.windowOpen;
        return result;
    }

    ImGui::SetNextWindowSize(ImVec2(820.0f, 560.0f), ImGuiCond_FirstUseEver);
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

    ImGui::Text(u8c(u8"FPS: %d / セル列: %d / 一時入力マス: %d"), data.fps, static_cast<int>(data.cells.size()), static_cast<int>(state.entries.size()));
    drawSelectedCellEditor(data, state, result);
    ImGui::Separator();

    const int baseColumns = 1 + std::max(1, static_cast<int>(data.cells.size()));
    const int instructionColumns = state.showInstructionColumns ? 4 : 0;
    const int columnCount = baseColumns + instructionColumns;

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders |
                                  ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_Resizable |
                                  ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_ScrollX |
                                  ImGuiTableFlags_SizingFixedFit;

    if (ImGui::BeginTable("FormalTimesheetEditTable", columnCount, flags, ImVec2(0.0f, 0.0f))) {
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
                const bool rowSelected = frame == state.selectedTimelineFrame;
                ImGui::TableNextRow(ImGuiTableRowFlags_None, state.rowHeight);

                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(frame);
                if (ImGui::Selectable(std::to_string(frame + 1).c_str(), rowSelected)) {
                    if (state.selectedTimelineFrame != frame) {
                        state.selectedTimelineFrame = frame;
                        result.timelineFrameChanged = true;
                    }
                }
                ImGui::PopID();

                const int cellColumnCount = std::max(1, static_cast<int>(data.cells.size()));
                for (int cellColumn = 0; cellColumn < cellColumnCount; ++cellColumn) {
                    ImGui::TableSetColumnIndex(1 + cellColumn);

                    const bool hasRealCell = cellColumn < static_cast<int>(data.cells.size());
                    const std::string cellId = hasRealCell ? cellIdForColumn(data, cellColumn) : std::string{};
                    const bool selectedCell = hasRealCell &&
                        frame == state.selectedTimelineFrame &&
                        cellColumn == state.selectedCellColumn;
                    const TimesheetPanelEditableEntry* entry = hasRealCell ? findEntry(state, frame, cellId) : nullptr;
                    const std::string label = hasRealCell ? displayLabelForEntry(entry, selectedCell) : u8c(u8"—");

                    if (selectedCell) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(90, 110, 160, 90));
                    }

                    ImGui::PushID(cellColumn);
                    if (ImGui::Selectable(label.c_str(), selectedCell)) {
                        if (state.selectedTimelineFrame != frame) {
                            state.selectedTimelineFrame = frame;
                            result.timelineFrameChanged = true;
                        }
                        if (state.selectedCellColumn != cellColumn) {
                            state.selectedCellColumn = cellColumn;
                            result.cellSelectionChanged = true;
                        }
                        if (entry != nullptr && kindUsesDrawingNumber(entry->kind)) {
                            state.editDrawingFrameNumber = std::max(1, entry->drawingFrameNumber);
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        if (entry == nullptr) {
                            ImGui::SetTooltip(u8c(u8"未記入。クリックで選択。上のボタンで入力。"));
                        } else {
                            ImGui::SetTooltip(
                                u8c(u8"%s / 作画F%d"),
                                entryKindLabel(entry->kind),
                                std::max(0, entry->drawingFrameNumber));
                        }
                    }
                    ImGui::PopID();
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

    normalizeSelection(data, state);
    result.selectedTimelineFrame = state.selectedTimelineFrame;
    result.selectedCellColumn = state.selectedCellColumn;
    ImGui::End();
    return result;
}

} // namespace perapera::ui
