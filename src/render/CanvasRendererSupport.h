#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <imgui.h>

#include "core/Frame.h"
#include "core/Layer.h"
#include "core/Stroke.h"
#include "render/CanvasBitmap.h"
#include "render/CanvasRenderer.h"

namespace perapera::canvas_renderer {

constexpr float kMinZoom = 0.05f;
constexpr float kMaxZoom = 32.0f;
constexpr float kCanvasBorderThickness = 1.0f;
constexpr std::size_t kMaxCachedLayerBitmaps = 128U;
constexpr std::size_t kMaxCachedOnionBitmaps = 24U;

std::uint8_t alphaByte(float opacity);
float displayOpacityForLayer(const Layer& layer, CanvasDisplayMode displayMode);
int displayLayerRank(LayerType type);
ImU32 colorWithAlpha(float r, float g, float b, float a);
void hashCombine(std::uint64_t& seed, std::uint64_t value);
std::uint64_t hashFloat(float value);
void bakeStrokeByEngine(CanvasBitmap& bitmap, const Stroke& stroke, float opacity);
std::string frameCacheId(const Frame& frame, int frameIndex);
std::string layerCacheId(const Layer& layer, int layerIndex);
std::uint64_t layerRevisionValue(const Layer& layer);
bool paintAppendOrderIsSafe(const Layer& layer, std::size_t fromIndex);

} // namespace perapera::canvas_renderer
