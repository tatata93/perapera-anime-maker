// このファイルの役割:
// Project の初期値生成とセル検索を実装する。

#include "core/Project.h"

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
