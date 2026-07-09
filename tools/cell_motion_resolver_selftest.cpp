#include "core/CellMotionResolver.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace perapera;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

bool nearlyEqual(float a, float b)
{
    return std::fabs(a - b) < 0.001f;
}

} // namespace

int main()
{
    Cell cell;
    cell.placement.x = 1.0f;
    cell.placement.y = 2.0f;
    cell.placement.scale = 1.0f;
    cell.placement.rotation = 0.0f;

    CellMotionKey key2;
    key2.frame = 2;
    key2.placement.x = 10.0f;
    key2.placement.y = 20.0f;
    key2.placement.scale = 1.0f;
    key2.placement.rotation = 0.0f;
    key2.interpolation = "linear";
    cell.motionKeys.push_back(key2);

    CellMotionKey key4;
    key4.frame = 4;
    key4.placement.x = 20.0f;
    key4.placement.y = 40.0f;
    key4.placement.scale = 2.0f;
    key4.placement.rotation = 90.0f;
    key4.interpolation = "linear";
    cell.motionKeys.push_back(key4);

    const CellPlacement t1 = resolveCellPlacementAtFrame(cell, 1);
    require(nearlyEqual(t1.x, 1.0f), "before first key uses base x");

    const CellPlacement t2 = resolveCellPlacementAtFrame(cell, 2);
    require(nearlyEqual(t2.x, 10.0f), "at key uses key x");

    const CellPlacement t3 = resolveCellPlacementAtFrame(cell, 3);
    require(nearlyEqual(t3.x, 15.0f), "linear x halfway");
    require(nearlyEqual(t3.scale, 1.5f), "linear scale halfway");
    require(nearlyEqual(t3.rotation, 45.0f), "linear rotation halfway");

    const CellPlacement t5 = resolveCellPlacementAtFrame(cell, 5);
    require(nearlyEqual(t5.x, 20.0f), "after last key holds last x");

    CellMotionKey key6;
    key6.frame = 6;
    key6.placement.x = 60.0f;
    key6.interpolation = "hold";
    cell.motionKeys.push_back(key6);

    CellMotionKey key8;
    key8.frame = 8;
    key8.placement.x = 80.0f;
    key8.interpolation = "linear";
    cell.motionKeys.push_back(key8);

    const CellPlacement t7 = resolveCellPlacementAtFrame(cell, 7);
    require(nearlyEqual(t7.x, 60.0f), "hold interpolation keeps previous key");

    std::cout << "perapera_cell_motion_resolver_selftest passed\n";
    return 0;
}
