// このファイルの役割:
// 正式タイムシートv1の中核データモデルに対する小さな補助処理を実装する。

#include "core/Timesheet.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace perapera {
namespace {

std::string toString(std::string_view value)
{
    return std::string(value.begin(), value.end());
}

} // namespace

const char* timesheetEntryTypeToString(TimesheetEntryType type) noexcept
{
    switch (type) {
    case TimesheetEntryType::Hold:
        return "hold";
    case TimesheetEntryType::Drawing:
        return "drawing";
    case TimesheetEntryType::Null:
        return "null";
    case TimesheetEntryType::Key:
        return "key";
    case TimesheetEntryType::Inbetween:
        return "inbetween";
    }
    return "hold";
}

TimesheetEntryType timesheetEntryTypeFromString(std::string_view value) noexcept
{
    if (value == "drawing") {
        return TimesheetEntryType::Drawing;
    }
    if (value == "null") {
        return TimesheetEntryType::Null;
    }
    if (value == "key") {
        return TimesheetEntryType::Key;
    }
    if (value == "inbetween") {
        return TimesheetEntryType::Inbetween;
    }
    return TimesheetEntryType::Hold;
}

const char* timesheetEntryTypeJapaneseLabel(TimesheetEntryType type) noexcept
{
    switch (type) {
    case TimesheetEntryType::Hold:
        return "ホールド";
    case TimesheetEntryType::Drawing:
        return "作画";
    case TimesheetEntryType::Null:
        return "空セル";
    case TimesheetEntryType::Key:
        return "原画";
    case TimesheetEntryType::Inbetween:
        return "中割";
    }
    return "ホールド";
}

bool timesheetEntryTypeUsesDrawingNumber(TimesheetEntryType type) noexcept
{
    return type == TimesheetEntryType::Drawing ||
        type == TimesheetEntryType::Key ||
        type == TimesheetEntryType::Inbetween;
}

int normalizeTimesheetFrameCount(int totalFrames) noexcept
{
    return std::max(1, totalFrames);
}

int clampTimesheetFrame(const Timesheet& timesheet, int timelineFrame) noexcept
{
    return std::clamp(timelineFrame, 1, normalizeTimesheetFrameCount(timesheet.totalFrames));
}

TimesheetCellTrack* findTimesheetTrack(Timesheet& timesheet, std::string_view cellId) noexcept
{
    const auto it = std::find_if(
        timesheet.tracks.begin(),
        timesheet.tracks.end(),
        [&](const TimesheetCellTrack& track) {
            return track.cellId == cellId;
        });
    return it == timesheet.tracks.end() ? nullptr : &(*it);
}

const TimesheetCellTrack* findTimesheetTrack(const Timesheet& timesheet, std::string_view cellId) noexcept
{
    const auto it = std::find_if(
        timesheet.tracks.begin(),
        timesheet.tracks.end(),
        [&](const TimesheetCellTrack& track) {
            return track.cellId == cellId;
        });
    return it == timesheet.tracks.end() ? nullptr : &(*it);
}

TimesheetCellTrack& ensureTimesheetTrack(Timesheet& timesheet, std::string_view cellId, std::string_view displayName)
{
    if (TimesheetCellTrack* existing = findTimesheetTrack(timesheet, cellId)) {
        if (!displayName.empty() && existing->displayName.empty()) {
            existing->displayName = toString(displayName);
        }
        return *existing;
    }

    TimesheetCellTrack track;
    track.cellId = toString(cellId);
    track.displayName = toString(displayName);
    timesheet.tracks.push_back(std::move(track));
    return timesheet.tracks.back();
}

void sortTimesheetTrackEntries(TimesheetCellTrack& track)
{
    std::stable_sort(
        track.entries.begin(),
        track.entries.end(),
        [](const TimesheetEntry& left, const TimesheetEntry& right) {
            return left.timelineFrame < right.timelineFrame;
        });
}

void dedupeTimesheetTrackEntriesByTimelineFrame(TimesheetCellTrack& track)
{
    std::vector<TimesheetEntry> deduped;
    deduped.reserve(track.entries.size());

    for (const TimesheetEntry& entry : track.entries) {
        if (!deduped.empty() && deduped.back().timelineFrame == entry.timelineFrame) {
            deduped.back() = entry;
            continue;
        }
        deduped.push_back(entry);
    }

    track.entries = std::move(deduped);
}

void normalizeTimesheet(Timesheet& timesheet)
{
    timesheet.totalFrames = normalizeTimesheetFrameCount(timesheet.totalFrames);
    timesheet.defaultExposure = std::max(1, timesheet.defaultExposure);

    for (TimesheetCellTrack& track : timesheet.tracks) {
        track.defaultExposure = std::max(1, track.defaultExposure);
        for (TimesheetEntry& entry : track.entries) {
            entry.timelineFrame = clampTimesheetFrame(timesheet, entry.timelineFrame);
            if (!timesheetEntryTypeUsesDrawingNumber(entry.type)) {
                entry.drawingFrameNumber = 0;
            } else {
                entry.drawingFrameNumber = std::max(1, entry.drawingFrameNumber);
            }
        }
        sortTimesheetTrackEntries(track);
        dedupeTimesheetTrackEntriesByTimelineFrame(track);
    }

    for (TimesheetFrameNote& note : timesheet.frameNotes) {
        note.timelineFrame = clampTimesheetFrame(timesheet, note.timelineFrame);
    }
}

} // namespace perapera
