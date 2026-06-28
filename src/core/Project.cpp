// このファイルの役割:
// Project の初期値生成とセル検索を実装する。

#include "core/Project.h"
#include "core/TimesheetResolver.h" // Timesheet Step C: resolver header compile fence.

namespace perapera {

Project Project::createDefault()
{
    Project project;
    project.version = "1.0";
    project.name = "作品タイトル";
    project.canvas = WorkCanvas{};
    project.output = OutputSettings{};
    project.timeline = TimelineSettings{};
    project.camera = CameraSettings{};
    project.audio = AudioSettings{};

    Cell defaultCell = Cell::createDefault();
    project.cellOrder.push_back(defaultCell.id);
    project.cells.push_back(defaultCell);

    // Timesheet Step A:
    // 初期状態では全タイムラインフレームで、既定セルのF001をHold表示する。
    // Step Cでは、このTimesheetを読み取って表示対象フレームを解決する関数を追加した。
    project.timesheet = Timesheet::createDefault(project.cells, project.timeline.totalFrames);
    return project;
}

Cell* Project::cellById(const std::string& cellId)
{
    for (Cell& cell : cells) {
        if (cell.id == cellId) {
            return &cell;
        }
    }
    return nullptr;
}

const Cell* Project::cellById(const std::string& cellId) const
{
    for (const Cell& cell : cells) {
        if (cell.id == cellId) {
            return &cell;
        }
    }
    return nullptr;
}

} // namespace perapera
