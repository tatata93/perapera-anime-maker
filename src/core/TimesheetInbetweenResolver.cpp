// このファイルの役割:
// 原画を先に置き、その間に中割を作るための原画間検出を実装する。

#include "core/TimesheetInbetweenResolver.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace perapera {
namespace {

struct EntryWithIndex {
    const TimesheetEntry* entry = nullptr;
    std::size_t originalIndex = 0;
};

bool entryLess(const EntryWithIndex& left, const EntryWithIndex& right) noexcept
{
    if (left.entry->timelineFrame != right.entry->timelineFrame) {
        return left.entry->timelineFrame < right.entry->timelineFrame;
    }
    return left.originalIndex < right.originalIndex;
}

std::vector<EntryWithIndex> sortedEntriesOfType(const TimesheetCellTrack& track, TimesheetEntryType type)
{
    std::vector<EntryWithIndex> result;
    result.reserve(track.entries.size());
    for (std::size_t index = 0; index < track.entries.size(); ++index) {
        const TimesheetEntry& entry = track.entries[index];
        if (entry.type == type && entry.drawingFrameNumber > 0) {
            result.push_back(EntryWithIndex{&entry, index});
        }
    }
    std::stable_sort(result.begin(), result.end(), entryLess);
    return result;
}

TimesheetKeyDrawingEndpoint endpointFromEntry(const TimesheetEntry& entry) noexcept
{
    TimesheetKeyDrawingEndpoint endpoint;
    endpoint.found = true;
    endpoint.timelineFrame = entry.timelineFrame;
    endpoint.drawingFrameNumber = entry.drawingFrameNumber;
    return endpoint;
}

bool isEntryAtTimelineFrame(const EntryWithIndex& ref, int timelineFrame) noexcept
{
    return ref.entry != nullptr && ref.entry->timelineFrame == timelineFrame;
}

} // namespace

ResolvedTimesheetInbetweenInterval resolveTimesheetInbetweenInterval(
    const Timesheet& timesheet,
    std::string_view cellId,
    int selectedTimelineFrame)
{
    ResolvedTimesheetInbetweenInterval result;
    result.cellId = std::string(cellId.begin(), cellId.end());
    result.selectedTimelineFrame = clampTimesheetFrame(timesheet, selectedTimelineFrame);

    const TimesheetCellTrack* track = findTimesheetTrack(timesheet, cellId);
    if (track == nullptr) {
        return result;
    }

    const std::vector<EntryWithIndex> keyEntries = sortedEntriesOfType(*track, TimesheetEntryType::Key);
    if (keyEntries.empty()) {
        return result;
    }

    int previousKeyIndex = -1;
    int nextKeyIndex = -1;

    for (int index = 0; index < static_cast<int>(keyEntries.size()); ++index) {
        const TimesheetEntry& entry = *keyEntries[static_cast<std::size_t>(index)].entry;
        if (entry.timelineFrame <= result.selectedTimelineFrame) {
            previousKeyIndex = index;
        }
        if (entry.timelineFrame > result.selectedTimelineFrame) {
            nextKeyIndex = index;
            break;
        }
    }

    // 選択Tが最後の原画そのものなら、直前原画 -> 選択原画の完了済み区間を見せる。
    if (nextKeyIndex < 0 && previousKeyIndex > 0) {
        const TimesheetEntry& previousKey = *keyEntries[static_cast<std::size_t>(previousKeyIndex)].entry;
        if (previousKey.timelineFrame == result.selectedTimelineFrame) {
            nextKeyIndex = previousKeyIndex;
            previousKeyIndex -= 1;
        }
    }

    if (previousKeyIndex >= 0) {
        result.previousKey = endpointFromEntry(*keyEntries[static_cast<std::size_t>(previousKeyIndex)].entry);
    }
    if (nextKeyIndex >= 0) {
        result.nextKey = endpointFromEntry(*keyEntries[static_cast<std::size_t>(nextKeyIndex)].entry);
    }

    result.selectedIsKey = std::any_of(
        keyEntries.begin(),
        keyEntries.end(),
        [&](const EntryWithIndex& ref) { return isEntryAtTimelineFrame(ref, result.selectedTimelineFrame); });

    result.hasClosedKeyInterval = result.previousKey.found &&
        result.nextKey.found &&
        result.previousKey.timelineFrame < result.nextKey.timelineFrame;

    if (!result.hasClosedKeyInterval) {
        return result;
    }

    result.selectedInsideInterval = result.selectedTimelineFrame >= result.previousKey.timelineFrame &&
        result.selectedTimelineFrame <= result.nextKey.timelineFrame;
    result.selectedPositionNumerator = result.selectedTimelineFrame - result.previousKey.timelineFrame;
    result.selectedPositionDenominator = result.nextKey.timelineFrame - result.previousKey.timelineFrame;
    result.availableTimelineSlotCount = std::max(0, result.nextKey.timelineFrame - result.previousKey.timelineFrame - 1);

    const std::vector<EntryWithIndex> inbetweenEntries = sortedEntriesOfType(*track, TimesheetEntryType::Inbetween);
    for (const EntryWithIndex& ref : inbetweenEntries) {
        const TimesheetEntry& entry = *ref.entry;
        if (entry.timelineFrame <= result.previousKey.timelineFrame || entry.timelineFrame >= result.nextKey.timelineFrame) {
            continue;
        }

        TimesheetInbetweenDrawingInfo info;
        info.timelineFrame = entry.timelineFrame;
        info.drawingFrameNumber = entry.drawingFrameNumber;
        result.inbetweens.push_back(info);
        if (entry.timelineFrame == result.selectedTimelineFrame) {
            result.selectedIsInbetween = true;
        }
    }

    result.usedInbetweenCount = static_cast<int>(result.inbetweens.size());
    for (int index = 0; index < static_cast<int>(result.inbetweens.size()); ++index) {
        TimesheetInbetweenDrawingInfo& info = result.inbetweens[static_cast<std::size_t>(index)];
        info.inbetweenIndex = index + 1;
        info.inbetweenCount = result.usedInbetweenCount;
    }

    return result;
}

} // namespace perapera
