#include "core/CellOrderResolver.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace perapera;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

Cell makeCell(const std::string& id)
{
    Cell cell;
    cell.id = id;
    cell.name = id;
    return cell;
}

} // namespace

int main()
{
    Project project;
    project.cells = {
        makeCell("A"),
        makeCell("B"),
        makeCell("C"),
    };

    project.cellOrder = {"C", "A", "missing", "C"};
    std::vector<int> ordered = resolvedProjectCellOrderIndices(project);
    require(ordered.size() == 3U, "ordered size");
    require(ordered[0] == 2, "C first");
    require(ordered[1] == 0, "A second");
    require(ordered[2] == 1, "B appended");

    project.cellOrder.clear();
    ordered = resolvedProjectCellOrderIndices(project);
    require(ordered.size() == 3U, "fallback size");
    require(ordered[0] == 0, "fallback A");
    require(ordered[1] == 1, "fallback B");
    require(ordered[2] == 2, "fallback C");

    std::cout << "perapera_cell_order_resolver_selftest passed\n";
    return 0;
}
