#include "ui/panels/CellPanelModel.h"

#include <iostream>
#include <stdexcept>
#include <string>

using perapera::Cell;
using perapera::Frame;
using perapera::Layer;
using perapera::Project;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Cell makeCell(const std::string& id, const std::string& name, const std::string& category, int layerCount)
{
    Cell cell;
    cell.id = id;
    cell.name = name;
    cell.category = category;

    Frame frame;
    frame.name = "F001";
    for (int i = 0; i < layerCount; ++i) {
        Layer layer;
        layer.layerId = "legacy_layer_" + std::to_string(i + 1);
        frame.layers.push_back(layer);
    }
    cell.frames.push_back(frame);
    return cell;
}

void testCellOrderAndNames()
{
    Project project;
    project.cells.push_back(makeCell("cell_001", "A", "character", 2));
    project.cells.push_back(makeCell("cell_003", "", "fx", 1));
    project.cellOrder = {"stale_cell"};
    project.cells[0].zOrder = 4;
    project.cells[1].zOrder = -2;

    require(perapera::ui::rebuildCellOrderAndZ(project), "rebuildCellOrderAndZ should report repaired order");
    require(project.cellOrder.size() == 2, "cellOrder should contain each non-empty cell id");
    require(project.cellOrder[0] == "cell_001", "cellOrder should preserve cell 1 id");
    require(project.cellOrder[1] == "cell_003", "cellOrder should preserve cell 2 id");
    require(project.cells[0].zOrder == 0, "cell 1 zOrder should be rebuilt");
    require(project.cells[1].zOrder == 1, "cell 2 zOrder should be rebuilt");
    require(!perapera::ui::rebuildCellOrderAndZ(project), "second rebuild should be clean");

    perapera::ui::syncCellOrder(project, "cell_010");
    perapera::ui::syncCellOrder(project, "cell_010");
    require(project.cellOrder.size() == 3, "syncCellOrder should append once");
    require(project.cellOrder.back() == "cell_010", "syncCellOrder should append requested id");

    require(perapera::ui::displayNameForCell(project.cells[0], 0) == "A [character]", "display name should include category");
    require(perapera::ui::displayNameForCell(project.cells[1], 1) == "cell_003 [fx]", "display name should fall back to id");
    require(perapera::ui::layerCountForCell(project.cells[0]) == 2, "layerCountForCell should count all layers");
    require(perapera::ui::firstFrameLayerCount(project.cells[0]) == 2, "firstFrameLayerCount should count first frame");
    require(perapera::ui::makeUniqueCellId(project) == "cell_004", "makeUniqueCellId should skip existing ids");
    require(perapera::ui::makeDuplicateName(project.cells[0], 2) == "A Copy 2", "makeDuplicateName should use source name");
}

void testLayerIdRepair()
{
    Project project;
    project.cells.push_back(makeCell("cell-A", "A", "character", 2));
    project.cells.push_back(makeCell("", "", "", 1));

    const std::uint64_t before = project.cells[0].frames[0].layers[0].revisionCounter;
    require(perapera::ui::ensureCellScopedLayerIds(project), "ensureCellScopedLayerIds should repair legacy ids");
    require(project.cells[0].frames[0].layers[0].layerId ==
                perapera::ui::expectedLayerIdForCellFrame(project.cells[0], 0, 0, 0),
            "cell scoped layer id should match expected format");
    require(project.cells[1].frames[0].layers[0].layerId ==
                perapera::ui::expectedLayerIdForCellFrame(project.cells[1], 1, 0, 0),
            "empty cell id should use deterministic fallback");
    require(project.cells[0].frames[0].layers[0].revisionCounter == before + 1,
            "layer repair should touch revision");
    require(!perapera::ui::ensureCellScopedLayerIds(project), "second layer repair should be clean");

    project.cells[0].frames.push_back(Frame{});
    project.cells[0].frames[1].layers.push_back(Layer{});
    perapera::ui::reassignLayerIdsForCell(project.cells[0], 0);
    require(project.cells[0].frames[1].layers[0].layerId ==
                perapera::ui::expectedLayerIdForCellFrame(project.cells[0], 0, 1, 0),
            "reassignLayerIdsForCell should cover every frame");
}

} // namespace

int main()
{
    try {
        testCellOrderAndNames();
        testLayerIdRepair();
    } catch (const std::exception& e) {
        std::cerr << "perapera_cell_panel_model_selftest failed: " << e.what() << '\n';
        return 1;
    }

    std::cout << "perapera_cell_panel_model_selftest passed\n";
    return 0;
}
