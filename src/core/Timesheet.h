#pragma once

// このファイルの役割:
// セルごとの「どの作画フレームを、タイムライン上のどのフレームで表示するか」を表す。
// Phase 2-pre Timesheet Step A では、Scene/Cut移行を行わず Project 直下の仮Timesheetとして扱う。
// ここではデータ構造だけを追加し、保存/読み込み、UI、再生反映は後続Stepで実装する。

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "core/Cell.h"

namespace perapera {

enum class TimesheetExposureKind {
    Null,
    Hold,
    Key,
    Inbetween,
};

inline const char* timesheetExposureKindToString(TimesheetExposureKind kind) noexcept
{
    switch (kind) {
    case TimesheetExposureKind::Null:
        return "null";
    case TimesheetExposureKind::Hold:
        return "hold";
    case TimesheetExposureKind::Key:
        return "key";
    case TimesheetExposureKind::Inbetween:
        return "inbetween";
    }
    return "hold";
}

inline TimesheetExposureKind timesheetExposureKindFromString(std::string_view value) noexcept
{
    if (value == "null") {
        return TimesheetExposureKind::Null;
    }
    if (value == "key") {
        return TimesheetExposureKind::Key;
    }
    if (value == "inbetween") {
        return TimesheetExposureKind::Inbetween;
    }
    return TimesheetExposureKind::Hold;
}

struct TimesheetCellExposure {
    std::string cellId;

    // Cell::frames の添字。0-based。
    // 例: 0 は cell.frames[0]、つまり既存UI上の F001 に対応する。
    int drawingFrameIndex = 0;

    // Null の場合、そのタイムラインフレームではこのセルを表示しない。
    TimesheetExposureKind kind = TimesheetExposureKind::Hold;

    bool isVisible() const noexcept { return kind != TimesheetExposureKind::Null; }
};

struct TimesheetFrame {
    // タイムライン上のフレーム番号。0-based。
    int timelineFrame = 0;
    std::vector<TimesheetCellExposure> cellExposures;

    TimesheetCellExposure* exposureForCell(const std::string& cellId)
    {
        auto it = std::find_if(cellExposures.begin(), cellExposures.end(),
            [&](const TimesheetCellExposure& exposure) { return exposure.cellId == cellId; });
        return it == cellExposures.end() ? nullptr : &(*it);
    }

    const TimesheetCellExposure* exposureForCell(const std::string& cellId) const
    {
        auto it = std::find_if(cellExposures.begin(), cellExposures.end(),
            [&](const TimesheetCellExposure& exposure) { return exposure.cellId == cellId; });
        return it == cellExposures.end() ? nullptr : &(*it);
    }
};

struct Timesheet {
    int totalFrames = 144;
    std::vector<TimesheetFrame> frames;

    static Timesheet createDefault(const std::vector<Cell>& cells, int requestedTotalFrames)
    {
        Timesheet timesheet;
        timesheet.ensureFrameCount(requestedTotalFrames);
        for (const Cell& cell : cells) {
            timesheet.ensureCell(cell.id, 0);
        }
        return timesheet;
    }

    void ensureFrameCount(int requestedTotalFrames)
    {
        if (requestedTotalFrames < 1) {
            requestedTotalFrames = 1;
        }

        totalFrames = requestedTotalFrames;
        frames.resize(static_cast<std::size_t>(totalFrames));
        for (int i = 0; i < totalFrames; ++i) {
            frames[static_cast<std::size_t>(i)].timelineFrame = i;
        }
    }

    void ensureCell(const std::string& cellId, int defaultDrawingFrameIndex = 0)
    {
        if (cellId.empty()) {
            return;
        }
        if (defaultDrawingFrameIndex < 0) {
            defaultDrawingFrameIndex = 0;
        }

        for (TimesheetFrame& frame : frames) {
            if (frame.exposureForCell(cellId) != nullptr) {
                continue;
            }

            TimesheetCellExposure exposure;
            exposure.cellId = cellId;
            exposure.drawingFrameIndex = defaultDrawingFrameIndex;
            exposure.kind = TimesheetExposureKind::Hold;
            frame.cellExposures.push_back(exposure);
        }
    }

    TimesheetFrame* frameOrNull(int timelineFrame)
    {
        if (timelineFrame < 0 || timelineFrame >= static_cast<int>(frames.size())) {
            return nullptr;
        }
        return &frames[static_cast<std::size_t>(timelineFrame)];
    }

    const TimesheetFrame* frameOrNull(int timelineFrame) const
    {
        if (timelineFrame < 0 || timelineFrame >= static_cast<int>(frames.size())) {
            return nullptr;
        }
        return &frames[static_cast<std::size_t>(timelineFrame)];
    }

    TimesheetCellExposure* exposureOrNull(int timelineFrame, const std::string& cellId)
    {
        TimesheetFrame* frame = frameOrNull(timelineFrame);
        return frame == nullptr ? nullptr : frame->exposureForCell(cellId);
    }

    const TimesheetCellExposure* exposureOrNull(int timelineFrame, const std::string& cellId) const
    {
        const TimesheetFrame* frame = frameOrNull(timelineFrame);
        return frame == nullptr ? nullptr : frame->exposureForCell(cellId);
    }
};

} // namespace perapera
