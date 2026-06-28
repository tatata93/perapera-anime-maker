#pragma once

// このファイルの役割:
// Phase 2-pre Timesheet Step C:
// Project直下の仮Timesheetから、指定タイムラインフレームで各セルのどの作画フレームを表示するかを解決する。
// ここでは読み取り専用の解決関数だけを追加し、UI/描画/書き出しへの反映は後続Stepで行う。

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "core/Project.h"

namespace perapera {

struct ResolvedTimesheetCell {
    const Cell* cell = nullptr;
    const Frame* frame = nullptr;
    const TimesheetCellExposure* exposure = nullptr;

    int cellIndex = -1;
    int drawingFrameIndex = -1;
    TimesheetExposureKind kind = TimesheetExposureKind::Null;

    // CellPanel側のセル表示状態。
    bool cellVisible = false;

    // Timesheet側の露出状態。kind == Null の場合は false。
    bool timesheetVisible = false;

    // 実際に描画・再生・書き出し対象にできる状態。
    bool renderable = false;
};

struct ResolvedTimesheetFrame {
    int requestedTimelineFrame = 0;
    int timelineFrame = 0;
    std::vector<ResolvedTimesheetCell> cells;
};

struct TimesheetResolveOptions {
    // true の場合、非表示セルや Null 露出も結果へ残す。
    // UI監査やデバッグ表示では true、実描画・書き出しでは false を想定する。
    bool includeHiddenCells = false;

    // true の場合、zOrder昇順で並べる。
    // 同じzOrderなら Project::cellOrder / cells 上の順序を維持する。
    bool sortByZOrder = true;
};

namespace TimesheetResolver {

inline int timesheetFrameCount(const Project& project) noexcept
{
    if (!project.timesheet.frames.empty()) {
        return static_cast<int>(project.timesheet.frames.size());
    }
    if (project.timesheet.totalFrames > 0) {
        return project.timesheet.totalFrames;
    }
    if (project.timeline.totalFrames > 0) {
        return project.timeline.totalFrames;
    }
    return 1;
}

inline int clampTimelineFrame(const Project& project, int timelineFrame) noexcept
{
    const int count = timesheetFrameCount(project);
    if (count <= 1) {
        return 0;
    }
    return std::clamp(timelineFrame, 0, count - 1);
}

inline int clampDrawingFrameIndex(const Cell& cell, int drawingFrameIndex) noexcept
{
    if (cell.frames.empty()) {
        return -1;
    }
    return std::clamp(drawingFrameIndex, 0, static_cast<int>(cell.frames.size()) - 1);
}

inline int cellIndexById(const Project& project, const std::string& cellId) noexcept
{
    for (int i = 0; i < static_cast<int>(project.cells.size()); ++i) {
        if (project.cells[static_cast<std::size_t>(i)].id == cellId) {
            return i;
        }
    }
    return -1;
}

inline std::vector<int> orderedCellIndices(const Project& project)
{
    std::vector<int> result;
    result.reserve(project.cells.size());

    std::vector<bool> used(project.cells.size(), false);
    for (const std::string& cellId : project.cellOrder) {
        const int index = cellIndexById(project, cellId);
        if (index < 0) {
            continue;
        }
        const std::size_t uindex = static_cast<std::size_t>(index);
        if (used[uindex]) {
            continue;
        }
        used[uindex] = true;
        result.push_back(index);
    }

    for (int i = 0; i < static_cast<int>(project.cells.size()); ++i) {
        if (!used[static_cast<std::size_t>(i)]) {
            result.push_back(i);
        }
    }

    return result;
}

inline ResolvedTimesheetCell resolveCell(const Project& project, int timelineFrame, int cellIndex)
{
    ResolvedTimesheetCell resolved;
    resolved.cellIndex = cellIndex;

    if (cellIndex < 0 || cellIndex >= static_cast<int>(project.cells.size())) {
        return resolved;
    }

    const int safeTimelineFrame = clampTimelineFrame(project, timelineFrame);
    const Cell& cell = project.cells[static_cast<std::size_t>(cellIndex)];
    resolved.cell = &cell;
    resolved.cellVisible = cell.visible && cell.opacity > 0.0f;

    int drawingFrameIndex = 0;
    TimesheetExposureKind kind = TimesheetExposureKind::Hold;
    const TimesheetCellExposure* exposure = project.timesheet.exposureOrNull(safeTimelineFrame, cell.id);
    if (exposure != nullptr) {
        drawingFrameIndex = exposure->drawingFrameIndex;
        kind = exposure->kind;
    }

    resolved.exposure = exposure;
    resolved.kind = kind;
    resolved.timesheetVisible = kind != TimesheetExposureKind::Null;

    const int safeDrawingFrameIndex = clampDrawingFrameIndex(cell, drawingFrameIndex);
    resolved.drawingFrameIndex = safeDrawingFrameIndex;
    if (safeDrawingFrameIndex >= 0) {
        resolved.frame = cell.frameOrNull(safeDrawingFrameIndex);
    }

    resolved.renderable = resolved.cellVisible && resolved.timesheetVisible && resolved.frame != nullptr;
    return resolved;
}

inline ResolvedTimesheetCell resolveCellById(const Project& project, int timelineFrame, const std::string& cellId)
{
    return resolveCell(project, timelineFrame, cellIndexById(project, cellId));
}

inline bool compareResolvedCellDrawOrder(const ResolvedTimesheetCell& left, const ResolvedTimesheetCell& right) noexcept
{
    const int leftZ = left.cell == nullptr ? 0 : left.cell->zOrder;
    const int rightZ = right.cell == nullptr ? 0 : right.cell->zOrder;
    if (leftZ != rightZ) {
        return leftZ < rightZ;
    }

    // 同じzOrderの場合は stable_sort に任せ、orderedCellIndices() が作った順序を維持する。
    return false;
}

inline ResolvedTimesheetFrame resolveFrame(
    const Project& project,
    int timelineFrame,
    TimesheetResolveOptions options = {})
{
    ResolvedTimesheetFrame resolvedFrame;
    resolvedFrame.requestedTimelineFrame = timelineFrame;
    resolvedFrame.timelineFrame = clampTimelineFrame(project, timelineFrame);

    const std::vector<int> orderedIndices = orderedCellIndices(project);
    resolvedFrame.cells.reserve(orderedIndices.size());
    for (int cellIndex : orderedIndices) {
        ResolvedTimesheetCell resolvedCell = resolveCell(project, resolvedFrame.timelineFrame, cellIndex);
        if (options.includeHiddenCells || resolvedCell.renderable) {
            resolvedFrame.cells.push_back(resolvedCell);
        }
    }

    if (options.sortByZOrder) {
        std::stable_sort(resolvedFrame.cells.begin(), resolvedFrame.cells.end(), compareResolvedCellDrawOrder);
    }

    return resolvedFrame;
}

} // namespace TimesheetResolver
} // namespace perapera
