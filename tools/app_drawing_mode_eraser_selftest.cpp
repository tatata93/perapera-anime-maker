#include "ui/AppDrawingModeEraser.h"

#include <iostream>
#include <stdexcept>
#include <vector>

using perapera::Stroke;
using perapera::StrokePoint;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Stroke makeLine(float x0, float y0, float x1, float y1, float radius)
{
    Stroke stroke;
    stroke.radiusPx = radius;
    stroke.points.push_back(StrokePoint{x0, y0, 1.0f});
    stroke.points.push_back(StrokePoint{x1, y1, 1.0f});
    return stroke;
}

void testNoHitPreservesStroke()
{
    const Stroke stroke = makeLine(0.0f, 0.0f, 100.0f, 0.0f, 4.0f);
    const Stroke eraser = makeLine(0.0f, 50.0f, 100.0f, 50.0f, 6.0f);
    bool changed = false;
    const std::vector<Stroke> result = perapera::app_drawing::splitStrokeByEraser(stroke, eraser, 6.0f, changed);

    require(!changed, "no-hit eraser should not mark the stroke changed");
    require(result.size() == 1U, "no-hit eraser should keep one stroke");
    require(result.front().points.size() == stroke.points.size(), "no-hit eraser should preserve original point count");
}

void testCrossingEraserSplitsStroke()
{
    const Stroke stroke = makeLine(0.0f, 0.0f, 100.0f, 0.0f, 4.0f);
    const Stroke eraser = makeLine(50.0f, -20.0f, 50.0f, 20.0f, 6.0f);
    bool changed = false;
    const std::vector<Stroke> result = perapera::app_drawing::splitStrokeByEraser(stroke, eraser, 6.0f, changed);

    require(changed, "crossing eraser should mark the stroke changed");
    require(result.size() >= 2U, "crossing eraser should split the stroke into multiple parts");
    for (const Stroke& part : result) {
        require(!part.points.empty(), "split parts should not be empty");
        require(part.radiusPx == stroke.radiusPx, "split parts should preserve stroke settings");
    }
}

void testShortTapKeepsLocalGap()
{
    const Stroke stroke = makeLine(0.0f, 0.0f, 220.0f, 0.0f, 4.0f);
    Stroke eraser;
    eraser.radiusPx = 100.0f;
    eraser.points.push_back(StrokePoint{110.0f, 0.0f, 1.0f});

    bool changed = false;
    const std::vector<Stroke> result = perapera::app_drawing::splitStrokeByEraser(stroke, eraser, 100.0f, changed);

    require(changed, "large short tap should still edit a long stroke");
    require(!result.empty(), "large short tap should keep at least one surviving part");
}

void testEmptyInputsAreStable()
{
    Stroke stroke;
    Stroke eraser;
    bool changed = false;
    const std::vector<Stroke> result = perapera::app_drawing::splitStrokeByEraser(stroke, eraser, 6.0f, changed);

    require(!changed, "empty inputs should not mark changed");
    require(result.size() == 1U, "empty target stroke should be returned as-is");
}

} // namespace

int main()
{
    try {
        testNoHitPreservesStroke();
        testCrossingEraserSplitsStroke();
        testShortTapKeepsLocalGap();
        testEmptyInputsAreStable();
    } catch (const std::exception& e) {
        std::cerr << "perapera_app_drawing_mode_eraser_selftest failed: " << e.what() << '\n';
        return 1;
    }

    std::cout << "perapera_app_drawing_mode_eraser_selftest passed\n";
    return 0;
}
