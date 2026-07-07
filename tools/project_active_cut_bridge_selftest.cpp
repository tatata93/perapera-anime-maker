#include <cstdlib>
#include <iostream>
#include <string>

#include "core/ProjectActiveCutBridge.h"

namespace {

perapera::Cell makeCell(std::string id, std::string name, std::string category, int zOrder)
{
    perapera::Cell cell;
    cell.id = std::move(id);
    cell.name = std::move(name);
    cell.category = std::move(category);
    cell.zOrder = zOrder;
    return cell;
}

bool require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

} // namespace

int main()
{
    perapera::Project project;

    auto view = perapera::activeCutCells(project);
    if (!require(view.valid(), "active cut cells view is valid for an empty Project")) {
        return EXIT_FAILURE;
    }
    if (!require(view.cellCount() == 0U, "empty Project starts with zero active-cut cells")) {
        return EXIT_FAILURE;
    }
    if (!require(view.orderCount() == 0U, "empty Project starts with zero active-cut cell order entries")) {
        return EXIT_FAILURE;
    }

    perapera::Cell character = makeCell("cell_A", "A", "character", 10);
    perapera::Cell background = makeCell("cell_BG", "BG", "background", 0);
    perapera::Cell layout = makeCell("cell_layout", "Layout", "layout", 5);

    if (!require(perapera::ensureActiveCutCell(project, character) != nullptr, "character cell is added")) {
        return EXIT_FAILURE;
    }
    if (!require(perapera::ensureActiveCutCell(project, background) != nullptr, "background cell is added")) {
        return EXIT_FAILURE;
    }
    if (!require(perapera::ensureActiveCutCell(project, layout) != nullptr, "layout cell is added")) {
        return EXIT_FAILURE;
    }

    const auto constView = perapera::activeCutCells(static_cast<const perapera::Project&>(project));
    if (!require(constView.valid(), "const active cut cells view is valid")) {
        return EXIT_FAILURE;
    }
    if (!require(constView.cellCount() == 3U, "three cells are visible through active cut bridge")) {
        return EXIT_FAILURE;
    }
    if (!require(constView.orderCount() == 3U, "three cell-order entries are visible through active cut bridge")) {
        return EXIT_FAILURE;
    }
    if (!require(constView.cellById("cell_BG") != nullptr, "background cell is found by id")) {
        return EXIT_FAILURE;
    }
    if (!require(constView.cellById("cell_layout") != nullptr, "layout cell is found by id")) {
        return EXIT_FAILURE;
    }
    if (!require(constView.cellById("missing") == nullptr, "missing cell returns nullptr")) {
        return EXIT_FAILURE;
    }

    const std::size_t previousCellCount = project.cells.size();
    const std::size_t previousOrderCount = project.cellOrder.size();
    if (!require(perapera::ensureActiveCutCell(project, makeCell("cell_BG", "BG Duplicate", "background", 99)) != nullptr,
                 "duplicate background cell returns existing cell")) {
        return EXIT_FAILURE;
    }
    if (!require(project.cells.size() == previousCellCount, "duplicate cell id does not add a second cell")) {
        return EXIT_FAILURE;
    }
    if (!require(project.cellOrder.size() == previousOrderCount, "duplicate cell id does not add a second order entry")) {
        return EXIT_FAILURE;
    }

    if (!require(!perapera::ensureActiveCutCellOrder(project, "cell_BG"), "existing order entry is not duplicated")) {
        return EXIT_FAILURE;
    }
    if (!require(!perapera::ensureActiveCutCellOrder(project, "missing"), "missing cell id is not added to cell order")) {
        return EXIT_FAILURE;
    }

    std::cout << "project_active_cut_bridge_selftest passed\n";
    return EXIT_SUCCESS;
}
