// このファイルの役割:
// タイムシートTから、表示・出力で使うセル別作画Fリストを作る。

#include "core/TimesheetSceneResolver.h"

#include "core/CellMotionResolver.h"

#include <cstddef>

namespace perapera {

ResolvedTimesheetSceneFrame resolveTimesheetSceneFrame(
    const Timesheet& timesheet,
    const std::vector<TimesheetSceneCellInput>& inputs,
    int timelineFrame)
{
    ResolvedTimesheetSceneFrame scene;
    scene.timelineFrame = clampTimesheetFrame(timesheet, timelineFrame);
    scene.cells.reserve(inputs.size());

    for (const TimesheetSceneCellInput& input : inputs) {
        const Cell* cell = input.cell;
        if (cell == nullptr || cell->frames.empty()) {
            continue;
        }
        if (input.requireCellVisible && (!cell->visible || cell->opacity <= 0.0f)) {
            continue;
        }

        const ResolvedTimesheetCell resolved = resolveTimesheetCell(timesheet, cell->id, scene.timelineFrame);
        if (!resolved.visible || resolved.drawingFrameNumber <= 0) {
            continue;
        }

        const int frameIndex = resolved.drawingFrameNumber - 1;
        if (frameIndex < 0 || frameIndex >= static_cast<int>(cell->frames.size())) {
            continue;
        }

        ResolvedTimesheetSceneCell sceneCell;
        sceneCell.cell = cell;
        sceneCell.cellIndex = input.cellIndex;
        sceneCell.visible = true;
        sceneCell.drawingFrameNumber = resolved.drawingFrameNumber;
        sceneCell.drawingFrameIndex = frameIndex;
        sceneCell.placement = resolveCellPlacementAtFrame(*cell, scene.timelineFrame);
        sceneCell.sourceType = resolved.sourceType;
        scene.cells.push_back(sceneCell);
    }

    return scene;
}

} // namespace perapera
