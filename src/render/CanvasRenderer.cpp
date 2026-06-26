// このファイルの役割:
// CanvasRendererの実装。
// 1レイヤー=1枚のCanvasBitmapという仕様を守り、毎フレーム全ストロークをDrawListへ積まない。
// Phase 1.5 Step 3: Roughを半透明にし、LayerTypeを表示キャッシュの判定に含める。
// Phase 1.5 Step 19b: Drawing表示でもPaintを線画より下に描き、線画が塗りで隠れないようにする。
// Phase 1.5 Step 19d: Paintレイヤー内ではFillStrokeを先に焼き、線・手描きストロークを後から焼く。
//   FillStrokeのrevision hashはポインタではなく内容サンプルで計算する。
// Phase 1.5 Step 19j: 通常レイヤーキャッシュキーからFrame*を外し、再生中のFrameコピーでも再ベイクを避ける。

#include "render/CanvasRenderer.h"

#include "brush/MyPaintBrushEngine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

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
    // Paintは常に線画より下へ表示する。
    // FillStroke化後はPaintが正しい面塗りとして残るため、
    // Drawingモードでもレイヤー配列順のまま描くと、PaintがNormal/ColorTraceを隠す場合がある。
    // その結果、ドラッグ中のプレビューだけ見えて、確定後に線が消えたように見える。
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

std::uint64_t hashString(const char* value)
{
    return std::hash<std::string_view>{}(std::string_view(value));
}

void bakeStrokeByEngine(CanvasBitmap& bitmap, const Stroke& stroke, float opacity)
{
    const float strokeOpacity = std::clamp(stroke.opacity, 0.05f, 1.0f);
    const float combinedOpacity = std::clamp(opacity * strokeOpacity, 0.0f, 1.0f);

    // Step 14g:
    // MyPaintストロークをUndo/保存/読み込み/キャッシュ再構築後も同じ見た目で戻す。
    // ドラッグ中はApp側の逐次処理、再構築時はMyPaintBrushEngine::bakeStrokeの
    // begin/addPoint/end再生で復元する。
    if (stroke.brushEngine == StrokeBrushEngine::MyPaint) {
        MyPaintBrushEngine myPaintEngine;
        if (myPaintEngine.isLibraryAvailable()) {
            myPaintEngine.bakeStroke(bitmap, stroke, combinedOpacity);
            return;
        }
    }

    bitmap.bakeStroke(stroke, combinedOpacity);
}

std::string frameCacheId(const Frame& frame)
{
    // 現在のFrame型は少なくとも name を持つ。別ブランチにある frameId へ直接依存すると
    // 既存リポジトリでコンパイルできないため、ここでは論理フレーム名をキャッシュキーにする。
    // 再生時にFrameがコピーされても name は維持されるので、Frame* より安定する。
    return frame.name.empty() ? std::string{"frame"} : frame.name;
}

std::string layerCacheId(const Layer& layer, int layerIndex)
{
    // layerId は既存フィールド。空や重複に備えて、空の時だけindexで補完する。
    if (!layer.layerId.empty()) {
        return layer.layerId;
    }
    return std::string{"layer_"} + std::to_string(layerIndex);
}

} // namespace

bool CanvasRenderer::LayerCacheKey::operator==(const LayerCacheKey& other) const noexcept
{
    return frameId == other.frameId && layerId == other.layerId;
}

bool CanvasRenderer::OnionCacheKey::operator==(const OnionCacheKey& other) const noexcept
{
    return frameIndex == other.frameIndex && isPrevious == other.isPrevious;
}

std::size_t CanvasRenderer::LayerCacheKeyHash::operator()(const LayerCacheKey& key) const noexcept
{
    std::size_t seed = std::hash<std::string>{}(key.frameId);
    seed ^= std::hash<std::string>{}(key.layerId) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
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

    const LayerCacheKey key{frameCacheId(*activeFrameForDirectAccess_), layerCacheId(layer, layerIndex)};
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
                          CanvasDisplayMode displayMode,
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
        std::vector<int> layerIndices;
        layerIndices.reserve(frame.layers.size());
        for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
            layerIndices.push_back(layerIndex);
        }

        // Project のレイヤー配列自体は変更せず、表示順だけを変える。
        // Paintは面塗りなので、Drawing/ColoringのどちらでもNormal/ColorTraceより先に描く。
        // これで線画確定後にPaintレイヤーへ隠れて「線が消えた」ように見える問題を防ぐ。
        std::stable_sort(layerIndices.begin(), layerIndices.end(), [&frame](int a, int b) {
            const LayerType typeA = frame.layers[static_cast<std::size_t>(a)].type;
            const LayerType typeB = frame.layers[static_cast<std::size_t>(b)].type;
            return displayLayerRank(typeA) < displayLayerRank(typeB);
        });

        for (int layerIndex : layerIndices) {
            const Layer& layer = frame.layers[static_cast<std::size_t>(layerIndex)];
            const float displayOpacity = displayOpacityForLayer(layer, displayMode);
            if (!layer.visible || layer.opacity <= 0.0f || displayOpacity <= 0.0f) {
                continue;
            }

            rebuildLayerBitmapIfNeeded(frame, layerIndex, layer);
            CanvasBitmap& bitmap = bitmapForLayer(frame, layerIndex);

            // uploadIfDirty() の false は「更新不要」も含む。
            // 既にGPU上に有効なTextureがあるなら、dirtyでなくても毎フレーム描画する。
            bitmap.uploadIfDirty(renderer_);
            if (!bitmap.hasTexture()) {
                continue;
            }

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

    // オニオンスキンも通常レイヤーと同じく、更新不要でも既存Textureは描く。
    bitmap.uploadIfDirty(renderer_);
    if (!bitmap.hasTexture()) {
        return;
    }

    (void)opacity;
    drawBitmap(bitmap, view, areaMin, areaSize, drawList, IM_COL32(255, 255, 255, 255));
}

CanvasBitmap& CanvasRenderer::bitmapForLayer(const Frame& frame, int layerIndex)
{
    return layerBitmaps_.try_emplace(LayerCacheKey{frameCacheId(frame), layerCacheId(frame.layers[static_cast<std::size_t>(layerIndex)], layerIndex)}).first->second;
}

void CanvasRenderer::rebuildLayerBitmapIfNeeded(const Frame& frame, int layerIndex, const Layer& layer)
{
    CanvasBitmap& bitmap = bitmapForLayer(frame, layerIndex);
    const LayerCacheKey key{frameCacheId(frame), layerCacheId(layer, layerIndex)};
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

    if (layer.type == LayerType::Paint) {
        // Paintレイヤー内では、面塗りであるFillStrokeを必ず先に焼く。
        // その後にPaintレイヤー上の手描き線や旧Simpleストロークを焼くことで、
        // バケツ塗り後に同一レイヤー内の線が塗りで消える事故を防ぐ。
        for (const Stroke& stroke : layer.strokes) {
            if (stroke.brushEngine == StrokeBrushEngine::Fill) {
                bakeStrokeByEngine(bitmap, stroke, 1.0f);
            }
        }
        for (const Stroke& stroke : layer.strokes) {
            if (stroke.brushEngine != StrokeBrushEngine::Fill) {
                bakeStrokeByEngine(bitmap, stroke, 1.0f);
            }
        }
    } else {
        for (const Stroke& stroke : layer.strokes) {
            bakeStrokeByEngine(bitmap, stroke, 1.0f);
        }
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
            bakeStrokeByEngine(bitmap, onionStroke, opacity * displayOpacityForLayer(layer, CanvasDisplayMode::Drawing));
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

        if (stroke.brushEngine == StrokeBrushEngine::Fill) {
            hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmapWidth));
            hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmapHeight));
            hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmap.size()));
            // FillStroke の再ベイク判定では pointer address を絶対に使わない。
            // 先頭・中央・末尾だけでは、塗り範囲が変わっても同じ値になり得るため、
            // 最大64点を等間隔にサンプリングして内容hashに含める。
            if (!stroke.bitmap.empty()) {
                const std::size_t sampleCount = std::min<std::size_t>(64U, stroke.bitmap.size());
                const std::size_t lastIndex = stroke.bitmap.size() - 1U;
                for (std::size_t sample = 0; sample < sampleCount; ++sample) {
                    const std::size_t index = sampleCount <= 1U
                        ? 0U
                        : (sample * lastIndex) / (sampleCount - 1U);
                    hashCombine(seed, static_cast<std::uint64_t>(index));
                    hashCombine(seed, static_cast<std::uint64_t>(stroke.bitmap[index]));
                }
            }
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
