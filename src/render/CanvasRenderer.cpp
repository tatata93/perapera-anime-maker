// src/render/CanvasRenderer.cpp
//
// CanvasRendererは、core層のFrame/Layer/Strokeを表示用CanvasBitmapに変換し、
// ImGuiのDrawListへ貼り付ける。重いストローク走査はdirty時だけ行い、
// 毎フレームは既存Textureの表示を中心にする。

#include "render/CanvasRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>

namespace perapera {
namespace {

constexpr float kMinimumZoom = 0.05f;
constexpr float kMaximumZoom = 64.0f;
constexpr float kCanvasBorderThickness = 1.0f;

float safeZoom(float zoom) {
    return std::clamp(zoom, kMinimumZoom, kMaximumZoom);
}

std::uint8_t alphaByte(float opacity) {
    const float clamped = std::clamp(opacity, 0.0f, 1.0f);
    return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
}

ImU32 strokeColor(const Stroke& stroke, float opacity) {
    const float r = std::clamp(stroke.color[0], 0.0f, 1.0f);
    const float g = std::clamp(stroke.color[1], 0.0f, 1.0f);
    const float b = std::clamp(stroke.color[2], 0.0f, 1.0f);
    const float a = std::clamp(stroke.color[3] * opacity, 0.0f, 1.0f);

    return IM_COL32(static_cast<int>(std::lround(r * 255.0f)),
                    static_cast<int>(std::lround(g * 255.0f)),
                    static_cast<int>(std::lround(b * 255.0f)),
                    static_cast<int>(std::lround(a * 255.0f)));
}

std::uint64_t mixRevision(std::uint64_t seed, std::uint64_t value) {
    // boost::hash_combine と同系統の簡易ハッシュ。
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

} // namespace

ImVec2 CanvasView::canvasToScreen(float canvasX,
                                  float canvasY,
                                  ImVec2 areaMin,
                                  ImVec2 /*areaSize*/) const {
    const float z = safeZoom(zoom);
    return ImVec2(areaMin.x + panX + canvasX * z,
                  areaMin.y + panY + canvasY * z);
}

ImVec2 CanvasView::screenToCanvas(float screenX,
                                  float screenY,
                                  ImVec2 areaMin,
                                  ImVec2 /*areaSize*/) const {
    const float z = safeZoom(zoom);
    return ImVec2((screenX - areaMin.x - panX) / z,
                  (screenY - areaMin.y - panY) / z);
}

void CanvasRenderer::setRenderer(SDL_Renderer* renderer) {
    renderer_ = renderer;
}

void CanvasRenderer::setCanvasSize(int width, int height) {
    const int nextWidth = std::max(0, width);
    const int nextHeight = std::max(0, height);

    if (canvasWidth_ == nextWidth && canvasHeight_ == nextHeight) {
        return;
    }

    canvasWidth_ = nextWidth;
    canvasHeight_ = nextHeight;

    for (auto& [layerIndex, bitmap] : bitmaps_) {
        (void)layerIndex;
        bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    }

    for (auto& [frameIndex, bitmap] : onionBitmaps_) {
        (void)frameIndex;
        bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    }

    markAllDirty();
}

void CanvasRenderer::markDirty(int layerIndex) {
    if (layerIndex < 0) {
        return;
    }
    dirtyLayers_.insert(layerIndex);
}

void CanvasRenderer::markAllDirty() {
    allLayersDirty_ = true;
    dirtyLayers_.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::bakeStroke(int layerIndex, const Stroke& stroke, float opacity) {
    if (layerIndex < 0) {
        return;
    }

    CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
    ensureBitmapSize(bitmap);
    bitmap.bakeStroke(stroke, opacity);
    onionRevisions_.clear();
}

void CanvasRenderer::eraseCircle(int layerIndex, float cx, float cy, float radius) {
    if (layerIndex < 0) {
        return;
    }

    CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
    ensureBitmapSize(bitmap);
    bitmap.eraseCircle(cx, cy, radius);
    onionRevisions_.clear();
}

void CanvasRenderer::clearLayer(int layerIndex) {
    if (layerIndex < 0) {
        return;
    }

    CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
    ensureBitmapSize(bitmap);
    bitmap.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::draw(const Frame& frame,
                          int activeLayerIndex,
                          const Stroke* currentStroke,
                          float currentStrokeOpacity,
                          const CanvasView& view,
                          ImVec2 areaMin,
                          ImVec2 areaSize,
                          ImDrawList* drawList) {
    if (drawList == nullptr || canvasWidth_ <= 0 || canvasHeight_ <= 0) {
        return;
    }

    rebuildDirtyBitmaps(frame);
    drawCanvasBounds(view, areaMin, areaSize, drawList);

    const ImVec2 imageMin = view.canvasToScreen(0.0f, 0.0f, areaMin, areaSize);
    const ImVec2 imageMax = view.canvasToScreen(static_cast<float>(canvasWidth_),
                                                static_cast<float>(canvasHeight_),
                                                areaMin,
                                                areaSize);

    for (std::size_t i = 0; i < frame.layers.size(); ++i) {
        const Layer& layer = frame.layers[i];
        if (!layer.visible || layer.opacity <= 0.0f) {
            continue;
        }

        CanvasBitmap& bitmap = bitmapForLayer(static_cast<int>(i));
        ensureBitmapSize(bitmap);
        if (!bitmap.uploadIfDirty(renderer_) || !bitmap.hasTexture()) {
            continue;
        }

        const ImU32 tint = IM_COL32(255, 255, 255, alphaByte(layer.opacity));
        drawList->AddImage(bitmap.imTextureID(), imageMin, imageMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint);
    }

    if (currentStroke != nullptr && activeLayerIndex >= 0) {
        drawCurrentStrokePreview(*currentStroke,
                                 currentStrokeOpacity,
                                 view,
                                 areaMin,
                                 areaSize,
                                 drawList);
    }
}

void CanvasRenderer::drawOnionSkin(const Frame& frame,
                                   int frameIndex,
                                   bool isPrevious,
                                   float opacity,
                                   const CanvasView& view,
                                   ImVec2 areaMin,
                                   ImVec2 areaSize,
                                   ImDrawList* drawList) {
    if (drawList == nullptr || canvasWidth_ <= 0 || canvasHeight_ <= 0 || opacity <= 0.0f) {
        return;
    }

    const std::uint64_t revision = frameRevisionHint(frame);
    const auto revisionIt = onionRevisions_.find(frameIndex);
    if (revisionIt == onionRevisions_.end() || revisionIt->second != revision) {
        rebuildOnionBitmap(frameIndex, frame);
        onionRevisions_[frameIndex] = revision;
    }

    auto bitmapIt = onionBitmaps_.find(frameIndex);
    if (bitmapIt == onionBitmaps_.end()) {
        return;
    }

    CanvasBitmap& bitmap = bitmapIt->second;
    if (!bitmap.uploadIfDirty(renderer_) || !bitmap.hasTexture()) {
        return;
    }

    const ImVec2 imageMin = view.canvasToScreen(0.0f, 0.0f, areaMin, areaSize);
    const ImVec2 imageMax = view.canvasToScreen(static_cast<float>(canvasWidth_),
                                                static_cast<float>(canvasHeight_),
                                                areaMin,
                                                areaSize);

    const int a = static_cast<int>(alphaByte(opacity));
    const ImU32 tint = isPrevious ? IM_COL32(120, 170, 255, a) : IM_COL32(255, 120, 120, a);
    drawList->AddImage(bitmap.imTextureID(), imageMin, imageMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint);
}

CanvasBitmap& CanvasRenderer::bitmapForLayer(int layerIndex) {
    CanvasBitmap& bitmap = bitmaps_[layerIndex];
    ensureBitmapSize(bitmap);
    return bitmap;
}

void CanvasRenderer::ensureBitmapSize(CanvasBitmap& bitmap) {
    if (canvasWidth_ <= 0 || canvasHeight_ <= 0) {
        return;
    }

    if (bitmap.width() != canvasWidth_ || bitmap.height() != canvasHeight_) {
        bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    }
}

void CanvasRenderer::rebuildLayerBitmap(int layerIndex, const Layer& layer) {
    CanvasBitmap& bitmap = bitmapForLayer(layerIndex);
    bitmap.clear();

    if (!layer.visible || layer.opacity <= 0.0f) {
        return;
    }

    for (const Stroke& stroke : layer.strokes) {
        bitmap.bakeStroke(stroke, layer.opacity);
    }
}

void CanvasRenderer::rebuildDirtyBitmaps(const Frame& frame) {
    if (allLayersDirty_) {
        bitmaps_.clear();
        for (std::size_t i = 0; i < frame.layers.size(); ++i) {
            rebuildLayerBitmap(static_cast<int>(i), frame.layers[i]);
        }
        allLayersDirty_ = false;
        dirtyLayers_.clear();
        return;
    }

    for (int layerIndex : dirtyLayers_) {
        if (layerIndex >= 0 && static_cast<std::size_t>(layerIndex) < frame.layers.size()) {
            rebuildLayerBitmap(layerIndex, frame.layers[static_cast<std::size_t>(layerIndex)]);
        }
    }
    dirtyLayers_.clear();
}

std::uint64_t CanvasRenderer::frameRevisionHint(const Frame& frame) const {
    std::uint64_t seed = 1469598103934665603ULL;
    seed = mixRevision(seed, static_cast<std::uint64_t>(frame.layers.size()));

    for (const Layer& layer : frame.layers) {
        seed = mixRevision(seed, layer.visible ? 1ULL : 0ULL);
        seed = mixRevision(seed, static_cast<std::uint64_t>(std::lround(layer.opacity * 1000.0f)));
        seed = mixRevision(seed, static_cast<std::uint64_t>(layer.strokes.size()));

        for (const Stroke& stroke : layer.strokes) {
            seed = mixRevision(seed, static_cast<std::uint64_t>(std::lround(stroke.radiusPx * 1000.0f)));
            seed = mixRevision(seed, static_cast<std::uint64_t>(stroke.points.size()));
        }
    }

    return seed;
}

void CanvasRenderer::rebuildOnionBitmap(int frameIndex, const Frame& frame) {
    CanvasBitmap& bitmap = onionBitmaps_[frameIndex];
    ensureBitmapSize(bitmap);
    bitmap.clear();

    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f) {
            continue;
        }

        for (const Stroke& stroke : layer.strokes) {
            bitmap.bakeStroke(stroke, layer.opacity);
        }
    }
}

void CanvasRenderer::drawCanvasBounds(const CanvasView& view,
                                      ImVec2 areaMin,
                                      ImVec2 areaSize,
                                      ImDrawList* drawList) const {
    const ImVec2 canvasMin = view.canvasToScreen(0.0f, 0.0f, areaMin, areaSize);
    const ImVec2 canvasMax = view.canvasToScreen(static_cast<float>(canvasWidth_),
                                                 static_cast<float>(canvasHeight_),
                                                 areaMin,
                                                 areaSize);

    drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(255, 255, 255, 255));
    drawList->AddRect(canvasMin, canvasMax, IM_COL32(90, 90, 90, 255), 0.0f, 0, kCanvasBorderThickness);
}

void CanvasRenderer::drawCurrentStrokePreview(const Stroke& stroke,
                                              float opacity,
                                              const CanvasView& view,
                                              ImVec2 areaMin,
                                              ImVec2 areaSize,
                                              ImDrawList* drawList) const {
    if (stroke.points.empty()) {
        return;
    }

    const ImU32 color = strokeColor(stroke, opacity);

    if (stroke.points.size() == 1) {
        const StrokePoint& point = stroke.points.front();
        const ImVec2 center = view.canvasToScreen(point.x, point.y, areaMin, areaSize);
        const float radius = std::max(0.5f, stroke.radiusPx * std::clamp(point.pressure, 0.0f, 1.0f) * safeZoom(view.zoom));
        drawList->AddCircleFilled(center, radius, color, 16);
        return;
    }

    for (std::size_t i = 1; i < stroke.points.size(); ++i) {
        const StrokePoint& previous = stroke.points[i - 1];
        const StrokePoint& current = stroke.points[i];
        const ImVec2 p0 = view.canvasToScreen(previous.x, previous.y, areaMin, areaSize);
        const ImVec2 p1 = view.canvasToScreen(current.x, current.y, areaMin, areaSize);
        const float pressure = (std::clamp(previous.pressure, 0.0f, 1.0f) + std::clamp(current.pressure, 0.0f, 1.0f)) * 0.5f;
        const float thickness = std::max(1.0f, stroke.radiusPx * 2.0f * pressure * safeZoom(view.zoom));
        drawList->AddLine(p0, p1, color, thickness);
    }
}

} // namespace perapera
