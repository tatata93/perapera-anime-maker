#pragma once

// src/render/CanvasRenderer.h
//
// Frame内のLayerをCanvasBitmapへ焼き、ImGuiのビューポートへ表示する。
// UI入力やProject保存は知らず、表示用キャッシュと座標変換だけを担当する。

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "core/Frame.h"
#include "core/Stroke.h"
#include "render/CanvasBitmap.h"

namespace perapera {

// キャンバスを画面上へ表示するためのズーム・パン状態。
// Step 1-4でUI入力と接続するまでは、CanvasRendererの補助型として置く。
struct CanvasView {
    float panX = 0.0f;
    float panY = 0.0f;
    float zoom = 1.0f;

    ImVec2 canvasToScreen(float canvasX,
                          float canvasY,
                          ImVec2 areaMin,
                          ImVec2 areaSize) const;

    ImVec2 screenToCanvas(float screenX,
                          float screenY,
                          ImVec2 areaMin,
                          ImVec2 areaSize) const;
};

class CanvasRenderer {
public:
    CanvasRenderer() = default;
    ~CanvasRenderer() = default;

    CanvasRenderer(const CanvasRenderer&) = delete;
    CanvasRenderer& operator=(const CanvasRenderer&) = delete;

    CanvasRenderer(CanvasRenderer&&) noexcept = default;
    CanvasRenderer& operator=(CanvasRenderer&&) noexcept = default;

    void setRenderer(SDL_Renderer* renderer);
    void setCanvasSize(int width, int height);

    // ストローク変更、消しゴム、Undo/Redo後に呼ぶ。
    // dirty指定されたレイヤーだけ、次のdraw()でCanvasBitmapを再構築する。
    void markDirty(int layerIndex);
    void markAllDirty();

    // ストロークをビットマップへ焼く。ペン確定時に使う。
    void bakeStroke(int layerIndex, const Stroke& stroke, float opacity);
    void eraseCircle(int layerIndex, float cx, float cy, float radius);
    void clearLayer(int layerIndex);

    // 毎フレーム呼ぶ描画メイン。
    // currentStrokeはペン入力中のプレビューで、確定済みストロークとは別にDrawListで軽く描く。
    void draw(const Frame& frame,
              int activeLayerIndex,
              const Stroke* currentStroke,
              float currentStrokeOpacity,
              const CanvasView& view,
              ImVec2 areaMin,
              ImVec2 areaSize,
              ImDrawList* drawList);

    // オニオンスキン表示。
    // isPrevious=trueなら青系、falseなら赤系の色を乗せる。
    void drawOnionSkin(const Frame& frame,
                       int frameIndex,
                       bool isPrevious,
                       float opacity,
                       const CanvasView& view,
                       ImVec2 areaMin,
                       ImVec2 areaSize,
                       ImDrawList* drawList);

private:
    SDL_Renderer* renderer_ = nullptr;
    int canvasWidth_ = 0;
    int canvasHeight_ = 0;

    std::unordered_map<int, CanvasBitmap> bitmaps_;
    std::unordered_set<int> dirtyLayers_;
    bool allLayersDirty_ = true;

    std::unordered_map<int, CanvasBitmap> onionBitmaps_;
    std::unordered_map<int, std::uint64_t> onionRevisions_;

    CanvasBitmap& bitmapForLayer(int layerIndex);
    void ensureBitmapSize(CanvasBitmap& bitmap);
    void rebuildLayerBitmap(int layerIndex, const Layer& layer);
    void rebuildDirtyBitmaps(const Frame& frame);

    std::uint64_t frameRevisionHint(const Frame& frame) const;
    void rebuildOnionBitmap(int frameIndex, const Frame& frame);

    void drawCanvasBounds(const CanvasView& view,
                          ImVec2 areaMin,
                          ImVec2 areaSize,
                          ImDrawList* drawList) const;

    void drawCurrentStrokePreview(const Stroke& stroke,
                                  float opacity,
                                  const CanvasView& view,
                                  ImVec2 areaMin,
                                  ImVec2 areaSize,
                                  ImDrawList* drawList) const;
};

} // namespace perapera
