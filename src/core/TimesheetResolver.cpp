// このファイルの役割:
// 正式タイムシートv1の表示解決処理を実装する。

#include "core/TimesheetResolver.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace perapera {
namespace {

struct EntryRef {
    const TimesheetEntry* entry = nullptr;
    std::size_t originalIndex = 0;
};

bool entryRefLess(const EntryRef& left, const EntryRef& right) noexcept
{
    if (left.entry->timelineFrame != right.entry->timelineFrame) {
        return left.entry->timelineFrame < right.entry->timelineFrame;
    }
    return left.originalIndex < right.originalIndex;
}

void applyEntryToResolvedCell(const TimesheetEntry& entry, ResolvedTimesheetCell& resolved)
{
    resolved.sourceType = entry.type;
    resolved.sourceEntry = &entry;

    switch (entry.type) {
    case TimesheetEntryType::Hold:
        // 現在の表示/非表示状態を維持する。
        return;
    case TimesheetEntryType::Null:
        resolved.visible = false;
        resolved.drawingFrameNumber = 0;
        return;
    case TimesheetEntryType::Drawing:
    case TimesheetEntryType::Key:
    case TimesheetEntryType::Inbetween:
        resolved.visible = entry.drawingFrameNumber > 0;
        resolved.drawingFrameNumber = resolved.visible ? entry.drawingFrameNumber : 0;
        return;
    }
}

} // namespace

ResolvedTimesheetCell resolveTimesheetCell(
    const Timesheet& timesheet,
    std::string_view cellId,
    int timelineFrame)
{
    ResolvedTimesheetCell resolved;
    resolved.cellId = std::string(cellId.begin(), cellId.end());

    const TimesheetCellTrack* track = findTimesheetTrack(timesheet, cellId);
    if (track == nullptr) {
        return resolved;
    }

    const int safeTimelineFrame = clampTimesheetFrame(timesheet, timelineFrame);
    std::vector<EntryRef> refs;
    refs.reserve(track->entries.size());

    for (std::size_t index = 0; index < track->entries.size(); ++index) {
        const TimesheetEntry& entry = track->entries[index];
        if (entry.timelineFrame <= safeTimelineFrame) {
            refs.push_back(EntryRef{&entry, index});
        }
    }

    std::stable_sort(refs.begin(), refs.end(), entryRefLess);
    for (const EntryRef& ref : refs) {
        applyEntryToResolvedCell(*ref.entry, resolved);
    }

    return resolved;
}

ResolvedTimesheetFrame resolveTimesheetFrame(
    const Timesheet& timesheet,
    int timelineFrame)
{
    ResolvedTimesheetFrame resolvedFrame;
    resolvedFrame.timelineFrame = clampTimesheetFrame(timesheet, timelineFrame);
    resolvedFrame.cells.reserve(timesheet.tracks.size());

    for (const TimesheetCellTrack& track : timesheet.tracks) {
        resolvedFrame.cells.push_back(resolveTimesheetCell(timesheet, track.cellId, resolvedFrame.timelineFrame));
    }

    return resolvedFrame;
}

} // namespace perapera
