#include "ui/AppDrawingModePreview.h"

#include <exception>
#include <iostream>
#include <stdexcept>

using perapera::Frame;
using perapera::Layer;
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

Frame makeSingleLayerFrame(const Stroke& stroke)
{
    Frame frame;
    Layer layer;
    layer.strokes.push_back(stroke);
    frame.layers.push_back(layer);
    return frame;
}

void testEmptyEraserReturnsCopy()
{
    const Stroke target = makeLine(0.0f, 0.0f, 100.0f, 0.0f, 4.0f);
    const Frame frame = makeSingleLayerFrame(target);
    const Stroke emptyEraser;

    const Frame preview = perapera::app_drawing::previewFrameWithEraser(frame, 0, emptyEraser);

    require(preview.layers.size() == 1U, "empty eraser should preserve layer count");
    require(preview.layers.front().strokes.size() == 1U, "empty eraser should preserve stroke count");
    require(frame.layers.front().strokes.size() == 1U, "source frame should remain unchanged");
}

void testInvalidLayerReturnsCopy()
{
    const Stroke target = makeLine(0.0f, 0.0f, 100.0f, 0.0f, 4.0f);
    const Stroke eraser = makeLine(50.0f, -20.0f, 50.0f, 20.0f, 6.0f);
    const Frame frame = makeSingleLayerFrame(target);

    const Frame preview = perapera::app_drawing::previewFrameWithEraser(frame, 3, eraser);

    require(preview.layers.size() == 1U, "invalid layer should preserve layer count");
    require(preview.layers.front().strokes.size() == 1U, "invalid layer should preserve stroke count");
}

void testCrossingEraserEditsPreviewOnly()
{
    const Stroke target = makeLine(0.0f, 0.0f, 100.0f, 0.0f, 4.0f);
    const Stroke eraser = makeLine(50.0f, -20.0f, 50.0f, 20.0f, 6.0f);
    const Frame frame = makeSingleLayerFrame(target);

    const Frame preview = perapera::app_drawing::previewFrameWithEraser(frame, 0, eraser);

    require(frame.layers.front().strokes.size() == 1U, "source frame should not be mutated");
    require(preview.layers.size() == 1U, "preview should preserve layer count");
    require(preview.layers.front().strokes.size() >= 2U, "crossing eraser should split the preview stroke");
}

} // namespace

int main()
{
    try {
        testEmptyEraserReturnsCopy();
        testInvalidLayerReturnsCopy();
        testCrossingEraserEditsPreviewOnly();
    } catch (const std::exception& e) {
        std::cerr << "perapera_app_drawing_mode_preview_selftest failed: " << e.what() << '\n';
        return 1;
    }

    std::cout << "perapera_app_drawing_mode_preview_selftest passed\n";
    return 0;
}
