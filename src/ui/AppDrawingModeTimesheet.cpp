#include "ui/AppDrawingModeTimesheet.h"

#include <algorithm>
#include <cstddef>

#include "core/Frame.h"
#include "core/TimesheetResolver.h"

namespace perapera::app_drawing {

ui::TimesheetPanelViewModel buildTimesheetPanelViewModel(const Project& project)
{
    ui::TimesheetPanelViewModel data;
    data.totalFrames = project.timeline.totalFrames > 0 ? project.timeline.totalFrames : 1;
    data.fps = 24;
    data.cells.reserve(project.cells.size());

    for (const Cell& timesheetCell : project.cells) {
        ui::TimesheetPanelCellColumn column;
        column.cellId = timesheetCell.id;
        column.displayName = timesheetCell.name;
        column.category = timesheetCell.category;
        data.cells.push_back(column);
    }

    return data;
}
int countTimesheetEntries(const Timesheet& timesheet)
{
    int count = 0;
    for (const TimesheetCellTrack& track : timesheet.tracks) {
        count += static_cast<int>(track.entries.size());
    }
    return count;
}

bool timesheetPanelKindUsesDrawingFrame(ui::TimesheetPanelEntryKind kind)
{
    return kind == ui::TimesheetPanelEntryKind::Drawing ||
        kind == ui::TimesheetPanelEntryKind::Key ||
        kind == ui::TimesheetPanelEntryKind::Inbetween;
}

Cell* projectCellById(Project& project, const std::string& cellId, int* cellIndexOut)
{
    for (int cellIndex = 0; cellIndex < static_cast<int>(project.cells.size()); ++cellIndex) {
        Cell& cell = project.cells[static_cast<std::size_t>(cellIndex)];
        if (cell.id == cellId) {
            if (cellIndexOut != nullptr) {
                *cellIndexOut = cellIndex;
            }
            return &cell;
        }
    }
    return nullptr;
}

const ui::TimesheetPanelEditableEntry* selectedTimesheetPanelEntry(
    const ui::TimesheetPanelViewModel& data,
    const ui::TimesheetPanelState& state)
{
    if (data.cells.empty()) {
        return nullptr;
    }

    const int safeColumn = std::clamp(
        state.selectedCellColumn,
        0,
        static_cast<int>(data.cells.size()) - 1);
    const std::string& cellId = data.cells[static_cast<std::size_t>(safeColumn)].cellId;
    for (const ui::TimesheetPanelEditableEntry& entry : state.entries) {
        if (entry.timelineFrame == state.selectedTimelineFrame && entry.cellId == cellId) {
            return &entry;
        }
    }
    return nullptr;
}

int autoCreateMissingDrawingFramesForTimesheetEntries(
    Project& project,
    const ui::TimesheetPanelState& state)
{
    int createdCount = 0;
    constexpr int kMaximumAutoCreateDrawingFrame = 240;

    for (const ui::TimesheetPanelEditableEntry& entry : state.entries) {
        if (!timesheetPanelKindUsesDrawingFrame(entry.kind) || entry.drawingFrameNumber <= 0) {
            continue;
        }
        if (entry.drawingFrameNumber > kMaximumAutoCreateDrawingFrame) {
            continue;
        }

        Cell* cell = projectCellById(project, entry.cellId);
        if (cell == nullptr) {
            continue;
        }

        while (static_cast<int>(cell->frames.size()) < entry.drawingFrameNumber) {
            const int newFrameNumber = static_cast<int>(cell->frames.size()) + 1;
            cell->frames.push_back(Frame::createDefault(newFrameNumber));
            ++createdCount;
        }
    }

    return createdCount;
}

int clampTimesheetPreviewFrame(int totalFrames, int zeroBasedFrame)
{
    const int lastFrame = std::max(0, totalFrames - 1);
    return std::clamp(zeroBasedFrame, 0, lastFrame);
}

TimesheetPlaybackStep stepTimesheetPreviewFrameIndex(int totalFrames,
                                                     int selectedFrame,
                                                     int delta,
                                                     bool pingPong,
                                                     int currentDirection)
{
    const int frameCount = std::max(1, totalFrames);
    TimesheetPlaybackStep result;
    result.requestedFrame = selectedFrame;
    result.direction = currentDirection;
    result.canPlay = frameCount > 1;
    if (!result.canPlay) {
        return result;
    }

    int nextFrame = selectedFrame + delta;
    if (nextFrame < 0 || nextFrame >= frameCount) {
        if (pingPong) {
            result.direction = -currentDirection;
            nextFrame = selectedFrame + result.direction;
        } else {
            nextFrame = nextFrame >= frameCount ? 0 : frameCount - 1;
        }
    }

    result.requestedFrame = nextFrame;
    return result;
}

TimesheetPlaybackRange normalizeTimesheetPlaybackRange(int totalFrames,
                                                       int startFrame,
                                                       int endFrame)
{
    const int lastFrame = std::max(0, totalFrames - 1);
    TimesheetPlaybackRange range;
    range.startFrame = std::clamp(startFrame, 0, lastFrame);
    range.endFrame = std::clamp(endFrame, 0, lastFrame);
    if (range.startFrame > range.endFrame) {
        std::swap(range.startFrame, range.endFrame);
    }
    return range;
}

TimesheetPlaybackStep stepTimesheetRangePreviewFrameIndex(int selectedFrame,
                                                          int delta,
                                                          TimesheetPlaybackRange range,
                                                          bool pingPong,
                                                          int currentDirection)
{
    TimesheetPlaybackStep result;
    result.requestedFrame = selectedFrame;
    result.direction = currentDirection;
    result.canPlay = true;

    if (range.endFrame <= range.startFrame) {
        result.requestedFrame = range.startFrame;
        return result;
    }

    int nextFrame = selectedFrame + delta;
    if (nextFrame < range.startFrame || nextFrame > range.endFrame) {
        if (pingPong) {
            result.direction = -currentDirection;
            nextFrame = selectedFrame + result.direction;
            nextFrame = std::clamp(nextFrame, range.startFrame, range.endFrame);
        } else {
            nextFrame = delta >= 0 ? range.startFrame : range.endFrame;
        }
    }

    result.requestedFrame = nextFrame;
    return result;
}

std::vector<int> buildTimesheetPlaybackOrderFrameIndicesForCell(const Timesheet& timesheet, const Cell& cell)
{
    std::vector<int> playbackOrderFrameIndices;
    if (countTimesheetEntries(timesheet) <= 0) {
        return playbackOrderFrameIndices;
    }

    const TimesheetCellTrack* track = findTimesheetTrack(timesheet, cell.id);
    if (track == nullptr) {
        return playbackOrderFrameIndices;
    }

    std::vector<TimesheetEntry> orderedEntries = track->entries;
    std::sort(orderedEntries.begin(),
              orderedEntries.end(),
              [](const TimesheetEntry& a, const TimesheetEntry& b) {
                  if (a.timelineFrame != b.timelineFrame) {
                      return a.timelineFrame < b.timelineFrame;
                  }
                  return static_cast<int>(a.type) < static_cast<int>(b.type);
              });

    std::vector<bool> used(cell.frames.size(), false);
    playbackOrderFrameIndices.reserve(orderedEntries.size());
    for (const TimesheetEntry& entry : orderedEntries) {
        if (!timesheetEntryTypeUsesDrawingNumber(entry.type)) {
            continue;
        }
        const int frameIndex = entry.drawingFrameNumber - 1;
        if (frameIndex < 0 || frameIndex >= static_cast<int>(cell.frames.size())) {
            continue;
        }
        if (used[static_cast<std::size_t>(frameIndex)]) {
            continue;
        }
        used[static_cast<std::size_t>(frameIndex)] = true;
        playbackOrderFrameIndices.push_back(frameIndex);
    }
    return playbackOrderFrameIndices;
}

int adjacentPlaybackOrderFrameIndex(const std::vector<int>& playbackOrderFrameIndices,
                                    int activeFrameIndex,
                                    int direction)
{
    if (direction == 0 || playbackOrderFrameIndices.empty()) {
        return -1;
    }

    const auto current = std::find(playbackOrderFrameIndices.begin(),
                                   playbackOrderFrameIndices.end(),
                                   activeFrameIndex);
    if (current == playbackOrderFrameIndices.end()) {
        return -1;
    }

    if (direction < 0) {
        if (current == playbackOrderFrameIndices.begin()) {
            return -1;
        }
        return *(current - 1);
    }

    const auto next = current + 1;
    if (next == playbackOrderFrameIndices.end()) {
        return -1;
    }
    return *next;
}

} // namespace perapera::app_drawing
