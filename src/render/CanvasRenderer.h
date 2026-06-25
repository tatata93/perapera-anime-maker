#pragma once

// このファイルの役割:
// CanvasBitmapをフレーム+レイヤー単位で管理し、ImGuiのDrawListへキャンバスを表示する。
// v14: オニオンキャッシュのキーからFrame*を外し、frameIndex+前後種別だけで管理する。
// 通常レイヤーはピクセルキャッシュを使い、描画中のストロークだけDrawListで軽く描く。

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "core/Frame.h"
#include "core/Stroke.h"
#include "render/CanvasBitmap.h"

namespace perapera {

// ビューポートのパン・ズーム状態。
// panX / panY は画面px単位の表示位置、zoom はキャンバスpxから画面pxへの倍率。
struct CanvasView {
    float panX = 0.0f;
    float panY = 0.0f;
    float zoom = 1.0f;

    ImVec2 canvasToScreen(float canvasX, float canvasY,
                          ImVec2 areaMin, ImVec2 areaSize) const;
    ImVec2 screenToCanvas(float screenX, float screenY,
                          ImVec2 areaMin, ImVec2 areaSize) const;

    void clampZoom();
};

class CanvasRenderer {
public:
    void setRenderer(SDL_Renderer* renderer);
    void setCanvasSize(int width, int height);

    // ストローク変更の通知。Phase 1では安全優先で全キャッシュを破棄する。
    void markDirty(int layerIndex);
    void markAllDirty();

    // 旧Step互換入口。現在はProject内のストローク点列を正本にし、次回drawで再構築する。
    void bakeStroke(int layerIndex, const Stroke& stroke, float opacity);

    // Simple互換の確定時焼き込み入口。現状はbakeStrokeと同じくキャッシュ再構築へ委譲する。
    void bakeStrokeOnLayer(int layerIndex, const Stroke& stroke, float opacity);

    void eraseCircle(int layerIndex, float canvasX, float canvasY, float radius);
    void clearLayer(int layerIndex);

    // MyPaint逐次描画用。直近draw()対象のアクティブFrame/LayerのCanvasBitmapを返す。
    CanvasBitmap* activeBitmap(int layerIndex);

    // 指定レイヤーのCanvasBitmapポインタを返す。
    // レイヤーが存在しない場合は nullptr を返す。
    CanvasBitmap* bitmapForLayerPtr(int layerIndex);

    // 逐次描画でCanvasBitmapへ直接描いたあと、Project側の点列追加とキャッシュrevisionを同期する。
    void markActiveBitmapClean(int layerIndex, const Layer& layer);

    void draw(const Frame& frame,
              int activeLayerIndex,
              const Stroke* currentStroke,
              float currentStrokeOpacity,
              const CanvasView& view,
              ImVec2 areaMin,
              ImVec2 areaSize,
              ImDrawList* drawList);

    void drawOnionSkin(const Frame& frame,
                       int frameIndex,
                       bool isPrevious,
                       float opacity,
                       const CanvasView& view,
                       ImVec2 areaMin,
                       ImVec2 areaSize,
                       ImDrawList* drawList);

private:
    struct LayerCacheKey {
        const Frame* frame = nullptr;
        int layerIndex = 0;

        bool operator==(const LayerCacheKey& other) const noexcept;
    };

    struct OnionCacheKey {
        int frameIndex = 0;
        bool isPrevious = false;

        bool operator==(const OnionCacheKey& other) const noexcept;
    };

    struct LayerCacheKeyHash {
        std::size_t operator()(const LayerCacheKey& key) const noexcept;
    };

    struct OnionCacheKeyHash {
        std::size_t operator()(const OnionCacheKey& key) const noexcept;
    };

    SDL_Renderer* renderer_ = nullptr;
    const Frame* activeFrameForDirectAccess_ = nullptr;
    int canvasWidth_ = 0;
    int canvasHeight_ = 0;

    std::unordered_map<LayerCacheKey, CanvasBitmap, LayerCacheKeyHash> layerBitmaps_;
    std::unordered_map<LayerCacheKey, std::uint64_t, LayerCacheKeyHash> layerRevisions_;
    std::unordered_map<OnionCacheKey, CanvasBitmap, OnionCacheKeyHash> onionBitmaps_;
    std::unordered_map<OnionCacheKey, std::uint64_t, OnionCacheKeyHash> onionRevisions_;

    CanvasBitmap& bitmapForLayer(const Frame& frame, int layerIndex);
    void rebuildLayerBitmapIfNeeded(const Frame& frame, int layerIndex, const Layer& layer);
    void rebuildOnionBitmapIfNeeded(const Frame& frame, int frameIndex, bool isPrevious, float opacity);

    std::uint64_t layerRevisionHash(const Layer& layer) const;
    std::uint64_t frameRevisionHash(const Frame& frame) const;

    void drawBitmap(CanvasBitmap& bitmap,
                    const CanvasView& view,
                    ImVec2 areaMin,
                    ImVec2 areaSize,
                    ImDrawList* drawList,
                    ImU32 tintColor);

    void drawCurrentStrokePreview(const Stroke& stroke,
                                  float opacity,
                                  const CanvasView& view,
                                  ImVec2 areaMin,
                                  ImVec2 areaSize,
                                  ImDrawList* drawList) const;
};

} // namespace perapera
