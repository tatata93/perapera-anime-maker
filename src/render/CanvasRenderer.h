#pragma once

// このファイルの役割:
// CanvasBitmapをフレーム+レイヤー単位で管理し、ImGuiのDrawListへキャンバスを表示する。
// v14: オニオンキャッシュのキーからFrame*を外し、frameIndex+前後種別だけで管理する。
// Phase 1.5 Step 19j: 通常レイヤーキャッシュキーからFrame*を外し、frame.name + layer.layerId で管理する。
// Phase 1.5 Step 19l: 再生時の毎フレーム全StrokePoint hashを廃止し、Layer::revisionCounterベースにする。
// Phase 1.5 Step 19n: 旧markAllDirty呼び出しでも既存Textureを保持し、ストローク確定時の全線消えを防ぐ。
// Phase 1.5 Step 19o: Simple確定直後の旧markAllDirtyを吸収し、必要なdirtyだけrevision無効化する。
// Phase 1.5 Step 19p / Step 20: markAllDirtyを通常キャッシュ破棄に使わず、append-only追い焼きと段階的再ベイクでUI停止を避ける。
// Phase 1.5 Step 21b: 再生中に次フレームを少量ずつ先読みし、初回表示時の重さを逃がす。
// 通常レイヤーはピクセルキャッシュを使い、描画中のストロークだけDrawListで軽く描く。

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "core/Frame.h"
#include "core/Stroke.h"
#include "render/CanvasBitmap.h"

namespace perapera {

// キャンバスの表示用途。
// Drawing は通常作画、Coloring は彩色用の参照表示ルールを使う。
enum class CanvasDisplayMode {
    Drawing,
    Coloring,
};

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

    // ストローク変更の通知。既存Textureは保持し、O(1) revision stampで必要なレイヤーだけ再構築する。
    void markDirty(int layerIndex);
    void markAllDirty();
    void clearLayerCaches();

    // 旧Step互換入口。現在はProject内のストローク点列を正本にし、次回drawで再構築する。
    void bakeStroke(int layerIndex, const Stroke& stroke, float opacity);

    // Simple互換の確定時焼き込み入口。可能なら現在のアクティブBitmapへ1本だけ追い焼きする。
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

    // 旧互換入口。frameIndexを渡せない呼び出し元ではFrame名を論理キーに使う。
    void draw(const Frame& frame,
              int activeLayerIndex,
              const Stroke* currentStroke,
              float currentStrokeOpacity,
              const CanvasView& view,
              CanvasDisplayMode displayMode,
              ImVec2 areaMin,
              ImVec2 areaSize,
              ImDrawList* drawList);

    // 再生時はframeIndexを渡し、ユーザー編集可能なFrame名の重複でキャッシュ衝突しないようにする。
    void draw(const Frame& frame,
              int frameIndex,
              int activeLayerIndex,
              const Stroke* currentStroke,
              float currentStrokeOpacity,
              const CanvasView& view,
              CanvasDisplayMode displayMode,
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

    // 再生中の先読み用。現在フレーム描画後に、次フレームの未構築レイヤーを少しだけ焼く。
    void warmFrameCache(const Frame& frame,
                        int frameIndex,
                        CanvasDisplayMode displayMode,
                        int layerBudget,
                        int strokeBudgetPerLayer);
    bool frameCacheReady(const Frame& frame,
                         int frameIndex,
                         CanvasDisplayMode displayMode) const;

private:
    struct LayerCacheKey {
        std::string frameId;
        std::string layerId;

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

    struct LayerRebuildState {
        std::uint64_t targetRevision = 0U;
        std::size_t nextStrokeIndex = 0U;
        int pass = 0; // Paint: 0=Fill, 1=non-Fill. Other layers: 1 pass only.
    };

    SDL_Renderer* renderer_ = nullptr;
    const Frame* activeFrameForDirectAccess_ = nullptr;
    std::string activeFrameCacheId_;
    int canvasWidth_ = 0;
    int canvasHeight_ = 0;

    // App側には歴史的に bakeStrokeOnLayer() 直後の markAllDirty() が残っている。
    // その呼び出しを全キャッシュ無効化として扱うと、Simpleストローク1本ごとに重くなるため、
    // 追い焼き済み直後の1回だけ旧markAllDirtyを吸収する。
    bool suppressNextAllDirty_ = false;

    std::unordered_map<LayerCacheKey, CanvasBitmap, LayerCacheKeyHash> layerBitmaps_;
    std::unordered_map<LayerCacheKey, std::uint64_t, LayerCacheKeyHash> layerRevisions_;
    std::unordered_map<LayerCacheKey, std::size_t, LayerCacheKeyHash> layerCachedStrokeCounts_;
    std::unordered_map<LayerCacheKey, LayerRebuildState, LayerCacheKeyHash> layerRebuildStates_;
    std::unordered_map<LayerCacheKey, std::uint64_t, LayerCacheKeyHash> layerLastUsed_;
    std::unordered_map<OnionCacheKey, CanvasBitmap, OnionCacheKeyHash> onionBitmaps_;
    std::unordered_map<OnionCacheKey, std::uint64_t, OnionCacheKeyHash> onionRevisions_;
    std::unordered_map<OnionCacheKey, std::uint64_t, OnionCacheKeyHash> onionLastUsed_;
    std::vector<int> displayLayerIndicesScratch_;
    std::uint64_t cacheUseClock_ = 0U;

    CanvasBitmap& bitmapForLayer(const std::string& frameId, const Frame& frame, int layerIndex);
    const std::vector<int>& displayLayerIndices(const Frame& frame);
    bool layerNeedsBitmapWork(const std::string& frameId, const Frame& frame, int layerIndex, const Layer& layer) const;
    void rebuildLayerBitmapIfNeeded(const std::string& frameId, const Frame& frame, int layerIndex, const Layer& layer, int strokeBudgetPerDraw = 16);
    bool appendMissingStrokesIfPossible(const std::string& frameId, const Frame& frame, int layerIndex, const Layer& layer, std::uint64_t revision);
    bool rebuildLayerBitmapProgressively(const std::string& frameId, const Frame& frame, int layerIndex, const Layer& layer, std::uint64_t revision, int strokeBudgetPerDraw);
    void rebuildOnionBitmapIfNeeded(const Frame& frame, int frameIndex, bool isPrevious, float opacity);
    void pruneLayerCache(const std::string& protectedFrameId);
    void pruneOnionCache(const OnionCacheKey& protectedKey);

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
