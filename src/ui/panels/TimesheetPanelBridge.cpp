// このファイルの役割:
// TimesheetPanelの一時編集状態と正式core Timesheetモデルの変換境界を実装する。
// この段階ではApp、保存、キャンバス表示、再生、出力には接続しない。

#include "ui/panels/TimesheetPanelBridge.h"

#include <algorithm>
#include <string>
#include <utility>

namespace perapera::ui {
namespace {

std::string fallbackCellIdForColumn(int cellColumn)
{
    return std::string{"__timesheet_panel_cell_"} + std::to_string(cellColumn);
}

std::string panelCellIdForColumn(const TimesheetPanelViewModel& data, int cellColumn)
{
    if (cellColumn < 0 || cellColumn >= static_cast<int>(data.cells.size())) {
        return {};
    }

    const TimesheetPanelCellColumn& cell = data.cells[static_cast<std::size_t>(cellColumn)];
    if (!cell.cellId.empty()) {
        return cell.cellId;
    }
    return fallbackCellIdForColumn(cellColumn);
}

std::string panelDisplayNameForColumn(const TimesheetPanelViewModel& data, int cellColumn)
{
    if (cellColumn < 0 || cellColumn >= static_cast<int>(data.cells.size())) {
        return {};
    }

    const TimesheetPanelCellColumn& cell = data.cells[static_cast<std::size_t>(cellColumn)];
    if (!cell.displayName.empty()) {
        return cell.displayName;
    }
    if (!cell.cellId.empty()) {
        return cell.cellId;
    }
    return std::string{"セル"} + std::to_string(cellColumn + 1);
}

bool panelKindUsesDrawingNumber(TimesheetPanelEntryKind kind) noexcept
{
    return kind == TimesheetPanelEntryKind::Drawing ||
        kind == TimesheetPanelEntryKind::Key ||
        kind == TimesheetPanelEntryKind::Inbetween;
}

bool isKnownTrackCell(const TimesheetPanelViewModel& data, const std::string& cellId)
{
    for (int column = 0; column < static_cast<int>(data.cells.size()); ++column) {
        if (panelCellIdForColumn(data, column) == cellId) {
            return true;
        }
    }
    return false;
}

} // namespace

TimesheetEntryType timesheetEntryTypeFromPanelKind(TimesheetPanelEntryKind kind) noexcept
{
    switch (kind) {
    case TimesheetPanelEntryKind::Drawing:
        return TimesheetEntryType::Drawing;
    case TimesheetPanelEntryKind::Null:
        return TimesheetEntryType::Null;
    case TimesheetPanelEntryKind::Key:
        return TimesheetEntryType::Key;
    case TimesheetPanelEntryKind::Inbetween:
        return TimesheetEntryType::Inbetween;
    case TimesheetPanelEntryKind::Empty:
    case TimesheetPanelEntryKind::Hold:
        break;
    }
    return TimesheetEntryType::Hold;
}

TimesheetPanelEntryKind panelKindFromTimesheetEntryType(TimesheetEntryType type) noexcept
{
    switch (type) {
    case TimesheetEntryType::Drawing:
        return TimesheetPanelEntryKind::Drawing;
    case TimesheetEntryType::Null:
        return TimesheetPanelEntryKind::Null;
    case TimesheetEntryType::Key:
        return TimesheetPanelEntryKind::Key;
    case TimesheetEntryType::Inbetween:
        return TimesheetPanelEntryKind::Inbetween;
    case TimesheetEntryType::Hold:
        break;
    }
    return TimesheetPanelEntryKind::Hold;
}

Timesheet buildTimesheetFromPanelState(
    const TimesheetPanelViewModel& data,
    const TimesheetPanelState& state)
{
    Timesheet timesheet;
    timesheet.totalFrames = normalizeTimesheetFrameCount(data.totalFrames);
    timesheet.defaultExposure = 1;

    for (int column = 0; column < static_cast<int>(data.cells.size()); ++column) {
        const std::string cellId = panelCellIdForColumn(data, column);
        if (cellId.empty()) {
            continue;
        }
        ensureTimesheetTrack(timesheet, cellId, panelDisplayNameForColumn(data, column));
    }

    for (const TimesheetPanelEditableEntry& panelEntry : state.entries) {
        if (panelEntry.kind == TimesheetPanelEntryKind::Empty || panelEntry.cellId.empty()) {
            continue;
        }
        if (panelEntry.timelineFrame < 0 || panelEntry.timelineFrame >= timesheet.totalFrames) {
            continue;
        }
        if (!isKnownTrackCell(data, panelEntry.cellId)) {
            continue;
        }

        TimesheetCellTrack& track = ensureTimesheetTrack(timesheet, panelEntry.cellId);

        TimesheetEntry entry;
        entry.timelineFrame = panelEntry.timelineFrame + 1;
        entry.type = timesheetEntryTypeFromPanelKind(panelEntry.kind);
        entry.drawingFrameNumber = panelKindUsesDrawingNumber(panelEntry.kind)
            ? std::max(1, panelEntry.drawingFrameNumber)
            : 0;
        track.entries.push_back(std::move(entry));
    }

    normalizeTimesheet(timesheet);
    return timesheet;
}

void replacePanelEntriesFromTimesheet(
    const Timesheet& timesheet,
    TimesheetPanelState& state)
{
    state.entries.clear();

    const int totalFrames = normalizeTimesheetFrameCount(timesheet.totalFrames);
    for (const TimesheetCellTrack& track : timesheet.tracks) {
        if (track.cellId.empty()) {
            continue;
        }

        for (const TimesheetEntry& entry : track.entries) {
            const int zeroBasedFrame = std::clamp(entry.timelineFrame, 1, totalFrames) - 1;

            TimesheetPanelEditableEntry panelEntry;
            panelEntry.timelineFrame = zeroBasedFrame;
            panelEntry.cellId = track.cellId;
            panelEntry.kind = panelKindFromTimesheetEntryType(entry.type);
            panelEntry.drawingFrameNumber = timesheetEntryTypeUsesDrawingNumber(entry.type)
                ? std::max(1, entry.drawingFrameNumber)
                : 0;
            state.entries.push_back(std::move(panelEntry));
        }
    }
}


bool normalizeTimesheetPanelStateForViewModel(
    const TimesheetPanelViewModel& data,
    TimesheetPanelState& state)
{
    bool changed = false;

    const int totalFrames = normalizeTimesheetFrameCount(data.totalFrames);
    const int previousTimelineFrame = state.selectedTimelineFrame;
    state.selectedTimelineFrame = std::clamp(state.selectedTimelineFrame, 0, totalFrames - 1);
    changed = changed || state.selectedTimelineFrame != previousTimelineFrame;

    const int previousCellColumn = state.selectedCellColumn;
    if (data.cells.empty()) {
        state.selectedCellColumn = 0;
    } else {
        state.selectedCellColumn = std::clamp(state.selectedCellColumn, 0, static_cast<int>(data.cells.size()) - 1);
    }
    changed = changed || state.selectedCellColumn != previousCellColumn;

    const int previousDrawingFrameNumber = state.editDrawingFrameNumber;
    state.editDrawingFrameNumber = std::max(1, state.editDrawingFrameNumber);
    changed = changed || state.editDrawingFrameNumber != previousDrawingFrameNumber;

    std::vector<TimesheetPanelEditableEntry> normalizedEntries;
    normalizedEntries.reserve(state.entries.size());

    for (TimesheetPanelEditableEntry entry : state.entries) {
        if (entry.kind == TimesheetPanelEntryKind::Empty || entry.cellId.empty()) {
            changed = true;
            continue;
        }
        if (entry.timelineFrame < 0 || entry.timelineFrame >= totalFrames) {
            changed = true;
            continue;
        }
        if (!isKnownTrackCell(data, entry.cellId)) {
            changed = true;
            continue;
        }

        if (panelKindUsesDrawingNumber(entry.kind)) {
            const int previous = entry.drawingFrameNumber;
            entry.drawingFrameNumber = std::max(1, entry.drawingFrameNumber);
            changed = changed || previous != entry.drawingFrameNumber;
        } else if (entry.drawingFrameNumber != 0) {
            entry.drawingFrameNumber = 0;
            changed = true;
        }

        const auto existing = std::find_if(
            normalizedEntries.begin(),
            normalizedEntries.end(),
            [&](const TimesheetPanelEditableEntry& existingEntry) {
                return existingEntry.timelineFrame == entry.timelineFrame &&
                    existingEntry.cellId == entry.cellId;
            });
        if (existing == normalizedEntries.end()) {
            normalizedEntries.push_back(std::move(entry));
        } else {
            *existing = std::move(entry);
            changed = true;
        }
    }

    std::stable_sort(
        normalizedEntries.begin(),
        normalizedEntries.end(),
        [](const TimesheetPanelEditableEntry& left, const TimesheetPanelEditableEntry& right) {
            if (left.timelineFrame != right.timelineFrame) {
                return left.timelineFrame < right.timelineFrame;
            }
            return left.cellId < right.cellId;
        });

    if (normalizedEntries.size() != state.entries.size()) {
        changed = true;
    }
    if (!changed && normalizedEntries.size() == state.entries.size()) {
        for (std::size_t i = 0; i < normalizedEntries.size(); ++i) {
            const TimesheetPanelEditableEntry& left = normalizedEntries[i];
            const TimesheetPanelEditableEntry& right = state.entries[i];
            if (left.timelineFrame != right.timelineFrame ||
                left.cellId != right.cellId ||
                left.kind != right.kind ||
                left.drawingFrameNumber != right.drawingFrameNumber) {
                changed = true;
                break;
            }
        }
    }

    if (changed) {
        state.entries = std::move(normalizedEntries);
    }
    return changed;
}

} // namespace perapera::ui
