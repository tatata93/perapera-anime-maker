#include "render/CanvasRendererSupport.h"

#include "brush/MyPaintBrushEngine.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace perapera::canvas_renderer {
namespace {

float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

void hashStrokeO1(std::uint64_t& seed, const Stroke& stroke)
{
    hashCombine(seed, static_cast<std::uint64_t>(stroke.brushEngine));
    hashCombine(seed, static_cast<std::uint64_t>(stroke.points.size()));
    hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmapX));
    hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmapY));
    hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmapWidth));
    hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmapHeight));
    hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmap.size()));
    hashCombine(seed, hashFloat(stroke.opacity));
    hashCombine(seed, hashFloat(stroke.radiusPx));
    hashCombine(seed, hashFloat(stroke.hardness));
    hashCombine(seed, hashFloat(stroke.spacing));
    hashCombine(seed, hashFloat(stroke.pressureToSize));
    hashCombine(seed, hashFloat(stroke.pressureToOpacity));
    hashCombine(seed, hashFloat(stroke.watercolorBleed));
    hashCombine(seed, hashFloat(stroke.colorMix));
    hashCombine(seed, hashFloat(stroke.dryRate));
    for (float component : stroke.color) {
        hashCombine(seed, hashFloat(component));
    }
    if (!stroke.points.empty()) {
        hashCombine(seed, hashFloat(stroke.points.front().x));
        hashCombine(seed, hashFloat(stroke.points.front().y));
        hashCombine(seed, hashFloat(stroke.points.front().pressure));
        hashCombine(seed, hashFloat(stroke.points.back().x));
        hashCombine(seed, hashFloat(stroke.points.back().y));
        hashCombine(seed, hashFloat(stroke.points.back().pressure));
    }
    if (!stroke.bitmap.empty()) {
        hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmap.front()));
        hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmap[stroke.bitmap.size() / 2U]));
        hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmap.back()));
    }
}

} // namespace

std::uint8_t alphaByte(float opacity)
{
    return static_cast<std::uint8_t>(std::lround(clamp01(opacity) * 255.0f));
}

float displayOpacityForLayer(const Layer& layer, CanvasDisplayMode displayMode)
{
    if (displayMode == CanvasDisplayMode::Coloring) {
        switch (layer.type) {
        case LayerType::Normal:
            return std::min(layer.opacity, 0.45f);
        case LayerType::ColorTrace:
            return std::min(layer.opacity, 0.65f);
        case LayerType::Paint:
            return layer.opacity;
        case LayerType::ShadowGuide:
            return std::min(layer.opacity, 0.30f);
        case LayerType::Rough:
            return std::min(layer.opacity, 0.25f);
        }
    }

    if (layer.type == LayerType::Rough) {
        return std::min(layer.opacity, 0.50f);
    }
    if (layer.type == LayerType::ShadowGuide) {
        return std::min(layer.opacity, 0.75f);
    }
    return layer.opacity;
}

int displayLayerRank(LayerType type)
{
    switch (type) {
    case LayerType::Paint:
        return 0;
    case LayerType::Rough:
        return 1;
    case LayerType::Normal:
        return 2;
    case LayerType::ColorTrace:
        return 3;
    case LayerType::ShadowGuide:
        return 4;
    }
    return 5;
}

ImU32 colorWithAlpha(float r, float g, float b, float a)
{
    const auto rb = static_cast<int>(std::lround(clamp01(r) * 255.0f));
    const auto gb = static_cast<int>(std::lround(clamp01(g) * 255.0f));
    const auto bb = static_cast<int>(std::lround(clamp01(b) * 255.0f));
    const auto ab = static_cast<int>(std::lround(clamp01(a) * 255.0f));
    return IM_COL32(rb, gb, bb, ab);
}

void hashCombine(std::uint64_t& seed, std::uint64_t value)
{
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}

std::uint64_t hashFloat(float value)
{
    return std::hash<int>{}(static_cast<int>(std::lround(value * 1000.0f)));
}

void bakeStrokeByEngine(CanvasBitmap& bitmap, const Stroke& stroke, float opacity)
{
    const float strokeOpacity = std::clamp(stroke.opacity, 0.05f, 1.0f);
    const float combinedOpacity = std::clamp(opacity * strokeOpacity, 0.0f, 1.0f);

    if (stroke.brushEngine == StrokeBrushEngine::MyPaint) {
        MyPaintBrushEngine myPaintEngine;
        if (myPaintEngine.isLibraryAvailable()) {
            myPaintEngine.bakeStroke(bitmap, stroke, combinedOpacity);
            return;
        }
    }

    bitmap.bakeStroke(stroke, combinedOpacity);
}

std::string frameCacheId(const Frame& frame, int frameIndex)
{
    if (frameIndex >= 0) {
        return std::string{"frame_index_"} + std::to_string(frameIndex);
    }
    return frame.name.empty() ? std::string{"frame"} : frame.name;
}

std::string layerCacheId(const Layer& layer, int layerIndex)
{
    if (!layer.layerId.empty()) {
        return layer.layerId;
    }
    return std::string{"layer_"} + std::to_string(layerIndex);
}

std::uint64_t layerRevisionValue(const Layer& layer)
{
    std::uint64_t seed = 1099511628211ULL;
    hashCombine(seed, layer.revisionCounter);
    hashCombine(seed, static_cast<std::uint64_t>(layer.type));
    hashCombine(seed, static_cast<std::uint64_t>(layer.strokes.size()));

    if (!layer.strokes.empty()) {
        hashStrokeO1(seed, layer.strokes.front());
        if (layer.strokes.size() >= 2U) {
            hashStrokeO1(seed, layer.strokes.back());
        }
    }

    return seed;
}

bool paintAppendOrderIsSafe(const Layer& layer, std::size_t fromIndex)
{
    if (layer.type != LayerType::Paint || fromIndex >= layer.strokes.size()) {
        return true;
    }

    bool appendedHasFill = false;
    for (std::size_t index = fromIndex; index < layer.strokes.size(); ++index) {
        if (layer.strokes[index].brushEngine == StrokeBrushEngine::Fill) {
            appendedHasFill = true;
            break;
        }
    }
    if (!appendedHasFill) {
        return true;
    }

    for (std::size_t index = 0; index < fromIndex; ++index) {
        if (layer.strokes[index].brushEngine != StrokeBrushEngine::Fill) {
            return false;
        }
    }
    return true;
}

} // namespace perapera::canvas_renderer
