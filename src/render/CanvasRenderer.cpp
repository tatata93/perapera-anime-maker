// このファイルの役割:
// CanvasRendererの実装。
// 1レイヤー=1枚のCanvasBitmapという仕様を守り、毎フレーム全ストロークをDrawListへ積まない。

#include "render/CanvasRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace perapera {
namespace {

constexpr float kMinZoom = 0.05f;
constexpr float kMaxZoom = 32.0f;
constexpr float kCanvasBorderThickness = 1.0f;

float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

std::uint8_t alphaByte(float opacity)
{
    return static_cast<std::uint8_t>(std::lround(clamp01(opacity) * 255.0f));
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

} // namespace

ImVec2 CanvasView::canvasToScreen(float canvasX, float canvasY,
                                  ImVec2 areaMin, ImVec2 areaSize) const
{
    (void)areaSize;
    const float safeZoom = std::clamp(zoom, kMinZoom, kMaxZoom);
    return ImVec2(areaMin.x + panX + canvasX * safeZoom,
                  areaMin.y + panY + canvasY * safeZoom);
}

ImVec2 CanvasView::screenToCanvas(float screenX, float screenY,
                                  ImVec2 areaMin, ImVec2 areaSize) const
{
    (void)areaSize;
    const float safeZoom = std::clamp(zoom, kMinZoom, kMaxZoom);
    return ImVec2((screenX - areaMin.x - panX) / safeZoom,
                  (screenY - areaMin.y - panY) / safeZoom);
}

void CanvasView::clampZoom()
{
    zoom = std::clamp(zoom, kMinZoom, kMaxZoom);
}

void CanvasRenderer::setRenderer(SDL_Renderer* renderer)
{
    if (renderer_ == renderer) {
        return;
    }

    // SDL_Textureは作成元SDL_Rendererに紐づく。
    // Rendererが変わったら古いTextureを持つBitmapを破棄する。
    renderer_ = renderer;
    bitmaps_.clear();
    onionBitmaps_.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::setCanvasSize(int width, int height)
{
    const int safeWidth = std::max(0, width);
    const int safeHeight = std::max(0, height);
    if (canvasWidth_ == safeWidth && canvasHeight_ == safeHeight) {
        return;
    }

    canvasWidth_ = safeWidth;
    canvasHeight_ = safeHeight;
    markAllDirty();
}

void CanvasRenderer::markDirty(int layerIndex)
{
    bitmaps_.erase(layerIndex);
    onionBitmaps_.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::markAllDirty()
{
    bitmaps_.clear();
    onionBitmaps_.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::bakeStroke(int layerIndex, const Stroke& stroke, float opacity)
{
    if (renderer_ == nullptr || canvasWidth_ <= 0 || canvasHeight_ <= 0) {
        return;
    }

    CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
    if (!bitmap.hasTexture() || bitmap.width() != canvasWidth_ || bitmap.height() != canvasHeight_) {
        bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    }

    bitmap.bakeStroke(stroke, opacity);
    onionBitmaps_.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::eraseCircle(int layerIndex, float canvasX, float canvasY, float radius)
{
    if (renderer_ == nullptr || canvasWidth_ <= 0 || canvasHeight_ <= 0) {
        return;
    }

    CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
    if (!bitmap.hasTexture() || bitmap.width() != canvasWidth_ || bitmap.height() != canvasHeight_) {
        bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    }

    bitmap.eraseCircle(canvasX, canvasY, radius);
    onionBitmaps_.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::clearLayer(int layerIndex)
{
    CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
    bitmap.clear();
    onionBitmaps_.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::draw(const Frame& frame,
                          int activeLayerIndex,
                          const Stroke* currentStroke,
                          float currentStrokeOpacity,
                          const CanvasView& view,
                          ImVec2 areaMin,
                          ImVec2 areaSize,
                          ImDrawList* drawList)
{
    if (drawList == nullptr) {
        return;
    }

    const ImVec2 areaMax(areaMin.x + areaSize.x, areaMin.y + areaSize.y);
    drawList->PushClipRect(areaMin, areaMax, true);

    const ImU32 canvasBackColor = IM_COL32(24, 24, 28, 255);
    drawList->AddRectFilled(areaMin, areaMax, canvasBackColor);

    if (renderer_ != nullptr && canvasWidth_ > 0 && canvasHeight_ > 0) {
        for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
            const Layer& layer = frame.layers[static_cast<std::size_t>(layerIndex)];
            if (!layer.visible || layer.opacity <= 0.0f) {
                continue;
            }

            rebuildLayerBitmapIfNeeded(layerIndex, layer);
            CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
            if (!bitmap.uploadIfDirty(renderer_)) {
                continue;
            }

            const ImU32 tint = IM_COL32(255, 255, 255, alphaByte(layer.opacity));
            drawBitmap(bitmap, layer.opacity, view, areaMin, areaSize, drawList, tint);
        }
    }

    if (currentStroke != nullptr && !currentStroke->empty()) {
        drawCurrentStrokePreview(*currentStroke, currentStrokeOpacity, view, areaMin, areaSize, drawList);
    }

    const ImVec2 canvasMin = view.canvasToScreen(0.0f, 0.0f, areaMin, areaSize);
    const ImVec2 canvasMax = view.canvasToScreen(static_cast<float>(canvasWidth_),
                                                 static_cast<float>(canvasHeight_),
                                                 areaMin, areaSize);
    drawList->AddRect(canvasMin, canvasMax, IM_COL32(95, 95, 105, 200), 0.0f, 0, kCanvasBorderThickness);

    (void)activeLayerIndex;
    drawList->PopClipRect();
}

void CanvasRenderer::drawOnionSkin(const Frame& frame,
                                   int frameIndex,
                                   bool isPrevious,
                                   float opacity,
                                   const CanvasView& view,
                                   ImVec2 areaMin,
                                   ImVec2 areaSize,
                                   ImDrawList* drawList)
{
    if (drawList == nullptr || renderer_ == nullptr || opacity <= 0.0f) {
        return;
    }

    rebuildOnionBitmapIfNeeded(frame, frameIndex);
    CanvasBitmap& bitmap = onionBitmaps_.at(frameIndex);
    if (!bitmap.uploadIfDirty(renderer_)) {
        return;
    }

    const std::uint8_t a = alphaByte(opacity);
    const ImU32 tint = isPrevious ? IM_COL32(80, 150, 255, a)
                                  : IM_COL32(255, 110, 110, a);
    drawBitmap(bitmap, opacity, view, areaMin, areaSize, drawList, tint);
}

CanvasBitmap& CanvasRenderer::bitmapForLayer(int layerIndex)
{
    return bitmaps_.try_emplace(layerIndex).first->second;
}

void CanvasRenderer::rebuildLayerBitmapIfNeeded(int layerIndex, const Layer& layer)
{
    CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
    if (bitmap.hasTexture() && bitmap.width() == canvasWidth_ && bitmap.height() == canvasHeight_) {
        return;
    }

    bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    bitmap.clear();
    for (const Stroke& stroke : layer.strokes) {
        bitmap.bakeStroke(stroke, 1.0f);
    }
}

void CanvasRenderer::rebuildOnionBitmapIfNeeded(const Frame& frame, int frameIndex)
{
    const std::uint64_t revision = frameRevisionHash(frame);
    auto bitmapIt = onionBitmaps_.find(frameIndex);
    const auto revisionIt = onionRevisions_.find(frameIndex);
    const bool needsRebuild = bitmapIt == onionBitmaps_.end()
        || revisionIt == onionRevisions_.end()
        || revisionIt->second != revision
        || !bitmapIt->second.hasTexture()
        || bitmapIt->second.width() != canvasWidth_
        || bitmapIt->second.height() != canvasHeight_;

    if (!needsRebuild) {
        return;
    }

    CanvasBitmap& bitmap = onionBitmaps_.try_emplace(frameIndex).first->second;
    bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    bitmap.clear();

    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f) {
            continue;
        }
        for (const Stroke& stroke : layer.strokes) {
            bitmap.bakeStroke(stroke, 1.0f);
        }
    }

    onionRevisions_[frameIndex] = revision;
}

void CanvasRenderer::drawBitmap(CanvasBitmap& bitmap,
                                float opacity,
                                const CanvasView& view,
                                ImVec2 areaMin,
                                ImVec2 areaSize,
                                ImDrawList* drawList,
                                ImU32 tintColor)
{
    (void)opacity;
    if (!bitmap.hasTexture() || drawList == nullptr) {
        return;
    }

    const ImVec2 imageMin = view.canvasToScreen(0.0f, 0.0f, areaMin, areaSize);
    const ImVec2 imageMax = view.canvasToScreen(static_cast<float>(bitmap.width()),
                                                static_cast<float>(bitmap.height()),
                                                areaMin, areaSize);
    drawList->AddImage(bitmap.imTextureID(), imageMin, imageMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tintColor);
}

void CanvasRenderer::drawCurrentStrokePreview(const Stroke& stroke,
                                              float opacity,
                                              const CanvasView& view,
                                              ImVec2 areaMin,
                                              ImVec2 areaSize,
                                              ImDrawList* drawList) const
{
    if (drawList == nullptr || stroke.points.empty()) {
        return;
    }

    const float alpha = stroke.color[3] * opacity;
    const ImU32 color = colorWithAlpha(stroke.color[0], stroke.color[1], stroke.color[2], alpha);
    const float radius = std::max(1.0f, stroke.radiusPx * std::clamp(view.zoom, kMinZoom, kMaxZoom));

    if (stroke.points.size() == 1U) {
        const StrokePoint& point = stroke.points.front();
        const ImVec2 screenPoint = view.canvasToScreen(point.x, point.y, areaMin, areaSize);
        drawList->AddCircleFilled(screenPoint, radius, color, 16);
        return;
    }

    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const ImVec2 p0 = view.canvasToScreen(previous.x, previous.y, areaMin, areaSize);
        const ImVec2 p1 = view.canvasToScreen(current.x, current.y, areaMin, areaSize);
        const float pressure = std::max(0.1f, (previous.pressure + current.pressure) * 0.5f);
        drawList->AddLine(p0, p1, color, radius * pressure * 2.0f);
    }
}

std::uint64_t CanvasRenderer::frameRevisionHash(const Frame& frame) const
{
    std::uint64_t seed = 1469598103934665603ULL;
    hashCombine(seed, static_cast<std::uint64_t>(frame.layers.size()));
    hashCombine(seed, static_cast<std::uint64_t>(frame.durationFrames));

    for (const Layer& layer : frame.layers) {
        hashCombine(seed, std::hash<std::string>{}(layer.layerId));
        hashCombine(seed, std::hash<std::string>{}(layer.name));
        hashCombine(seed, layer.visible ? 1ULL : 0ULL);
        hashCombine(seed, hashFloat(layer.opacity));
        hashCombine(seed, std::hash<std::string>{}(layer.blendMode));
        hashCombine(seed, static_cast<std::uint64_t>(layer.strokes.size()));

        for (const Stroke& stroke : layer.strokes) {
            hashCombine(seed, hashFloat(stroke.radiusPx));
            for (float component : stroke.color) {
                hashCombine(seed, hashFloat(component));
            }
            hashCombine(seed, static_cast<std::uint64_t>(stroke.points.size()));
            for (const StrokePoint& point : stroke.points) {
                hashCombine(seed, hashFloat(point.x));
                hashCombine(seed, hashFloat(point.y));
                hashCombine(seed, hashFloat(point.pressure));
            }
        }
    }

    return seed;
}

} // namespace perapera
