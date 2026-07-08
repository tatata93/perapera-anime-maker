#include "render/CanvasRendererSupport.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using perapera::CanvasDisplayMode;
using perapera::Frame;
using perapera::Layer;
using perapera::LayerType;
using perapera::Stroke;
using perapera::StrokeBrushEngine;
using perapera::StrokePoint;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Stroke makeStroke(float x, float y)
{
    Stroke stroke;
    stroke.brushEngine = StrokeBrushEngine::Simple;
    stroke.opacity = 0.8f;
    stroke.radiusPx = 4.0f;
    stroke.points.push_back(StrokePoint{x, y, 0.5f});
    stroke.points.push_back(StrokePoint{x + 10.0f, y + 5.0f, 0.8f});
    return stroke;
}

Stroke makeFillStroke(std::uint8_t sample)
{
    Stroke stroke;
    stroke.brushEngine = StrokeBrushEngine::Fill;
    stroke.opacity = 1.0f;
    stroke.bitmapX = 2;
    stroke.bitmapY = 3;
    stroke.bitmapWidth = 3;
    stroke.bitmapHeight = 1;
    stroke.bitmap = {1U, sample, 255U};
    return stroke;
}

void testDisplayHelpers()
{
    Layer layer;
    layer.opacity = 0.9f;

    layer.type = LayerType::Normal;
    require(perapera::canvas_renderer::displayOpacityForLayer(layer, CanvasDisplayMode::Coloring) == 0.45f,
            "normal coloring opacity should be capped");

    layer.type = LayerType::Paint;
    require(perapera::canvas_renderer::displayOpacityForLayer(layer, CanvasDisplayMode::Coloring) == 0.9f,
            "paint coloring opacity should be unchanged");

    layer.type = LayerType::Rough;
    require(perapera::canvas_renderer::displayOpacityForLayer(layer, CanvasDisplayMode::Drawing) == 0.5f,
            "rough drawing opacity should be capped");

    require(perapera::canvas_renderer::displayLayerRank(LayerType::Paint) <
                perapera::canvas_renderer::displayLayerRank(LayerType::Normal),
            "paint should be drawn before normal line layers");
    require(perapera::canvas_renderer::alphaByte(-1.0f) == 0U, "alphaByte should clamp low");
    require(perapera::canvas_renderer::alphaByte(0.5f) == 128U, "alphaByte should round mid alpha");
    require(perapera::canvas_renderer::alphaByte(2.0f) == 255U, "alphaByte should clamp high");
}

void testCacheIds()
{
    Frame frame;
    frame.name = "same_name";
    require(perapera::canvas_renderer::frameCacheId(frame, 3) == "frame_index_3",
            "frame index should override editable names");
    require(perapera::canvas_renderer::frameCacheId(frame, -1) == "same_name",
            "legacy frame cache id should use frame name");
    frame.name.clear();
    require(perapera::canvas_renderer::frameCacheId(frame, -1) == "frame",
            "empty frame name should use fallback id");

    Layer layer;
    layer.layerId = "line_a";
    require(perapera::canvas_renderer::layerCacheId(layer, 7) == "line_a",
            "layer id should be preferred");
    layer.layerId.clear();
    require(perapera::canvas_renderer::layerCacheId(layer, 7) == "layer_7",
            "empty layer id should use index fallback");
}

void testRevisionValue()
{
    Layer layer;
    layer.type = LayerType::Normal;
    layer.strokes.push_back(makeStroke(1.0f, 2.0f));
    const std::uint64_t base = perapera::canvas_renderer::layerRevisionValue(layer);

    layer.opacity = 0.1f;
    require(perapera::canvas_renderer::layerRevisionValue(layer) == base,
            "layer opacity should not force bitmap rebake revision");

    layer.strokes.back().points.back().pressure = 0.2f;
    const std::uint64_t changedPoint = perapera::canvas_renderer::layerRevisionValue(layer);
    require(changedPoint != base, "representative point changes should affect revision");

    layer.strokes.push_back(makeFillStroke(9U));
    const std::uint64_t fillBase = perapera::canvas_renderer::layerRevisionValue(layer);
    layer.strokes.back().bitmap[1] = 10U;
    require(perapera::canvas_renderer::layerRevisionValue(layer) != fillBase,
            "fill bitmap samples should affect revision");

    layer.touchRevision();
    require(perapera::canvas_renderer::layerRevisionValue(layer) != fillBase,
            "explicit layer revision should affect revision value");
}

void testPaintAppendOrder()
{
    Layer paint;
    paint.type = LayerType::Paint;
    paint.strokes.push_back(makeFillStroke(1U));
    paint.strokes.push_back(makeStroke(0.0f, 0.0f));
    require(perapera::canvas_renderer::paintAppendOrderIsSafe(paint, 1U),
            "appending line work above existing fills should be safe");

    Layer unsafePaint;
    unsafePaint.type = LayerType::Paint;
    unsafePaint.strokes.push_back(makeStroke(0.0f, 0.0f));
    unsafePaint.strokes.push_back(makeFillStroke(2U));
    require(!perapera::canvas_renderer::paintAppendOrderIsSafe(unsafePaint, 1U),
            "appending a fill above existing line work should not be append-baked");

    Layer normal;
    normal.type = LayerType::Normal;
    normal.strokes.push_back(makeStroke(0.0f, 0.0f));
    normal.strokes.push_back(makeFillStroke(3U));
    require(perapera::canvas_renderer::paintAppendOrderIsSafe(normal, 1U),
            "non-paint layers do not need paint append ordering checks");
}

} // namespace

int main()
{
    try {
        testDisplayHelpers();
        testCacheIds();
        testRevisionValue();
        testPaintAppendOrder();
    } catch (const std::exception& e) {
        std::cerr << "perapera_canvas_renderer_support_selftest failed: " << e.what() << '\n';
        return 1;
    }

    std::cout << "perapera_canvas_renderer_support_selftest passed\n";
    return 0;
}
