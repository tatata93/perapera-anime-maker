// このファイルの役割:
// CanvasRendererの実装。
// 1レイヤー=1枚のCanvasBitmapという仕様を守り、毎フレーム全ストロークをDrawListへ積まない。
// Phase 1.5 Step 3: Roughを半透明にし、LayerTypeを表示キャッシュの判定に含める。

#include "render/CanvasRenderer.h"

#include "brush/MyPaintBrushEngine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

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

// Rough は下書き用なので、表示上は常に半透明上限をかける。
// レイヤー自体の opacity を下げた場合は、その値を優先する。
float displayOpacityForLayer(const Layer& layer)
{
    if (layer.type == LayerType::Rough) {
        return std::min(layer.opacity, 0.50f);
    }
    if (layer.type == LayerType::ShadowGuide) {
        return std::min(layer.opacity, 0.75f);
    }
    return layer.opacity;
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

std::uint64_t hashString(const char* value)
{
    return std::hash<std::string_view>{}(std::string_view(value));
}

void bakeStrokeByEngine(CanvasBitmap& bitmap, const Stroke& stroke, float opacity)
{
    const float strokeOpacity = std::clamp(stroke.opacity, 0.05f, 1.0f);
    const float combinedOpacity = std::clamp(opacity * strokeOpacity, 0.0f, 1.0f);

    // Step 14c:
    // MyPaintはドラッグ中逐次描画でCanvasBitmapへ直接dabを入れる。
    // 保存データ・オニオン・読み込み後再構築では、確定時一括libmypaint処理を避けるためSimple互換で再ベイクする。
    // これで長いMyPaintストロークを持つレイヤーを再表示してもUIが止まりにくい。
    (void)stroke.brushEngine;
    bitmap.bakeStroke(stroke, combinedOpacity);
}

std::size_t pointerHash(const void* pointer)
{
    return std::hash<const void*>{}(pointer);
}

} // namespace

bool CanvasRenderer::LayerCacheKey::operator==(const LayerCacheKey& other) const noexcept
{
    return frame == other.frame && layerIndex == other.layerIndex;
}

bool CanvasRenderer::OnionCacheKey::operator==(const OnionCacheKey& other) const noexcept
{
    return frameIndex == other.frameIndex && isPrevious == other.isPrevious;
}

std::size_t CanvasRenderer::LayerCacheKeyHash::operator()(const LayerCacheKey& key) const noexcept
{
    std::size_t seed = pointerHash(key.frame);
    seed ^= std::hash<int>{}(key.layerIndex) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    return seed;
}

std::size_t CanvasRenderer::OnionCacheKeyHash::operator()(const OnionCacheKey& key) const noexcept
{
    std::size_t seed = std::hash<int>{}(key.frameIndex);
    seed ^= std::hash<bool>{}(key.isPrevious) + 0x85ebca6bU + (seed << 6U) + (seed >> 2U);
    return seed;
}

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

    renderer_ = renderer;
    markAllDirty();
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
    (void)layerIndex;
    markAllDirty();
}

void CanvasRenderer::markAllDirty()
{
    layerBitmaps_.clear();
    layerRevisions_.clear();
    onionBitmaps_.clear();
    onionRevisions_.clear();
}

void CanvasRenderer::bakeStroke(int layerIndex, const Stroke& stroke, float opacity)
{
    (void)layerIndex;
    (void)stroke;
    (void)opacity;
    markAllDirty();
}

void CanvasRenderer::bakeStrokeOnLayer(int layerIndex, const Stroke& stroke, float opacity)
{
    bakeStroke(layerIndex, stroke, opacity);
}

void CanvasRenderer::eraseCircle(int layerIndex, float canvasX, float canvasY, float radius)
{
    (void)layerIndex;
    (void)canvasX;
    (void)canvasY;
    (void)radius;
    markAllDirty();
}

void CanvasRenderer::clearLayer(int layerIndex)
{
    (void)layerIndex;
    markAllDirty();
}

CanvasBitmap* CanvasRenderer::activeBitmap(int layerIndex)
{
    if (activeFrameForDirectAccess_ == nullptr || renderer_ == nullptr || canvasWidth_ <= 0 || canvasHeight_ <= 0) {
        return nullptr;
    }
    if (layerIndex < 0 || layerIndex >= static_cast<int>(activeFrameForDirectAccess_->layers.size())) {
        return nullptr;
    }

    const Layer& layer = activeFrameForDirectAccess_->layers[static_cast<std::size_t>(layerIndex)];
    rebuildLayerBitmapIfNeeded(*activeFrameForDirectAccess_, layerIndex, layer);

    CanvasBitmap& bitmap = bitmapForLayer(*activeFrameForDirectAccess_, layerIndex);
    if (bitmap.width() != canvasWidth_ || bitmap.height() != canvasHeight_ || !bitmap.hasTexture()) {
        bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    }
    return &bitmap;
}

CanvasBitmap* CanvasRenderer::bitmapForLayerPtr(int layerIndex)
{
    return activeBitmap(layerIndex);
}

void CanvasRenderer::markActiveBitmapClean(int layerIndex, const Layer& layer)
{
    if (activeFrameForDirectAccess_ == nullptr || layerIndex < 0) {
        return;
    }

    const LayerCacheKey key{activeFrameForDirectAccess_, layerIndex};
    layerRevisions_[key] = layerRevisionHash(layer);

    // このフレームを前後オニオンとして使っているキャッシュは古くなる可能性がある。
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
    activeFrameForDirectAccess_ = &frame;

    if (drawList == nullptr) {
        return;
    }

    const ImVec2 areaMax(areaMin.x + areaSize.x, areaMin.y + areaSize.y);
    drawList->PushClipRect(areaMin, areaMax, true);
    // 背景はApp側で先に塗る。ここで再度塗ると、先に描いたオニオンスキンが隠れる。

    if (renderer_ != nullptr && canvasWidth_ > 0 && canvasHeight_ > 0) {
        for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
            const Layer& layer = frame.layers[static_cast<std::size_t>(layerIndex)];
            if (!layer.visible || layer.opacity <= 0.0f) {
                continue;
            }

            rebuildLayerBitmapIfNeeded(frame, layerIndex, layer);
            CanvasBitmap& bitmap = bitmapForLayer(frame, layerIndex);
            if (!bitmap.uploadIfDirty(renderer_)) {
                continue;
            }

            const float displayOpacity = displayOpacityForLayer(layer);
            drawBitmap(bitmap, view, areaMin, areaSize, drawList, IM_COL32(255, 255, 255, alphaByte(displayOpacity)));
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

    rebuildOnionBitmapIfNeeded(frame, frameIndex, isPrevious, opacity);
    CanvasBitmap& bitmap = onionBitmaps_.at(OnionCacheKey{frameIndex, isPrevious});
    if (!bitmap.uploadIfDirty(renderer_)) {
        return;
    }

    (void)opacity;
    drawBitmap(bitmap, view, areaMin, areaSize, drawList, IM_COL32(255, 255, 255, 255));
}

CanvasBitmap& CanvasRenderer::bitmapForLayer(const Frame& frame, int layerIndex)
{
    return layerBitmaps_.try_emplace(LayerCacheKey{&frame, layerIndex}).first->second;
}

void CanvasRenderer::rebuildLayerBitmapIfNeeded(const Frame& frame, int layerIndex, const Layer& layer)
{
    CanvasBitmap& bitmap = bitmapForLayer(frame, layerIndex);
    const LayerCacheKey key{&frame, layerIndex};
    const std::uint64_t revision = layerRevisionHash(layer);
    const auto revisionIt = layerRevisions_.find(key);
    const bool needsRebuild = revisionIt == layerRevisions_.end()
        || revisionIt->second != revision
        || !bitmap.hasTexture()
        || bitmap.width() != canvasWidth_
        || bitmap.height() != canvasHeight_;

    if (!needsRebuild) {
        return;
    }

    bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    bitmap.clear();
    for (const Stroke& stroke : layer.strokes) {
        bakeStrokeByEngine(bitmap, stroke, 1.0f);
    }
    layerRevisions_[key] = revision;
}

void CanvasRenderer::rebuildOnionBitmapIfNeeded(const Frame& frame, int frameIndex, bool isPrevious, float opacity)
{
    const OnionCacheKey key{frameIndex, isPrevious};
    CanvasBitmap& bitmap = onionBitmaps_.try_emplace(key).first->second;
    std::uint64_t revision = frameRevisionHash(frame);
    hashCombine(revision, isPrevious ? 1ULL : 0ULL);
    hashCombine(revision, hashFloat(opacity));
    const auto revisionIt = onionRevisions_.find(key);
    const bool needsRebuild = revisionIt == onionRevisions_.end()
        || revisionIt->second != revision
        || !bitmap.hasTexture()
        || bitmap.width() != canvasWidth_
        || bitmap.height() != canvasHeight_;

    if (!needsRebuild) {
        return;
    }

    bitmap.resize(renderer_, canvasWidth_, canvasHeight_);
    bitmap.clear();

    // 黒い線にImGuiのtintを掛けても青/赤にならないため、
    // オニオンスキン用Bitmapには専用色のStrokeとして焼き込む。
    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f) {
            continue;
        }
        for (const Stroke& stroke : layer.strokes) {
            Stroke onionStroke = stroke;
            if (isPrevious) {
                onionStroke.color = {0.20f, 0.58f, 1.0f, 1.0f};
            } else {
                onionStroke.color = {1.0f, 0.30f, 0.25f, 1.0f};
            }
            bakeStrokeByEngine(bitmap, onionStroke, opacity * displayOpacityForLayer(layer));
        }
    }
    onionRevisions_[key] = revision;
}

void CanvasRenderer::drawBitmap(CanvasBitmap& bitmap,
                                const CanvasView& view,
                                ImVec2 areaMin,
                                ImVec2 areaSize,
                                ImDrawList* drawList,
                                ImU32 tintColor)
{
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

    const float alpha = stroke.color[3] * std::clamp(stroke.opacity, 0.05f, 1.0f) * opacity;
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

std::uint64_t CanvasRenderer::layerRevisionHash(const Layer& layer) const
{
    std::uint64_t seed = 1099511628211ULL;
    hashCombine(seed, layer.visible ? 1ULL : 0ULL);
    hashCombine(seed, hashFloat(layer.opacity));
    hashCombine(seed, static_cast<std::uint64_t>(layer.type));
    hashCombine(seed, static_cast<std::uint64_t>(layer.strokes.size()));

    for (const Stroke& stroke : layer.strokes) {
        hashCombine(seed, hashFloat(stroke.radiusPx));
        hashCombine(seed, hashString(strokeBrushEngineToString(stroke.brushEngine)));
        hashCombine(seed, hashFloat(stroke.opacity));
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
        hashCombine(seed, static_cast<std::uint64_t>(stroke.points.size()));
        for (const StrokePoint& point : stroke.points) {
            hashCombine(seed, hashFloat(point.x));
            hashCombine(seed, hashFloat(point.y));
            hashCombine(seed, hashFloat(point.pressure));
        }
    }

    return seed;
}

std::uint64_t CanvasRenderer::frameRevisionHash(const Frame& frame) const
{
    std::uint64_t seed = 1469598103934665603ULL;
    hashCombine(seed, static_cast<std::uint64_t>(frame.layers.size()));
    hashCombine(seed, static_cast<std::uint64_t>(frame.durationFrames));

    for (const Layer& layer : frame.layers) {
        hashCombine(seed, layerRevisionHash(layer));
    }

    return seed;
}

} // namespace perapera
