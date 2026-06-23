#pragma once

// このファイルの役割:
// CanvasBitmapをレイヤー単位で管理し、ImGuiのDrawListへキャンバスを表示する。
// 通常レイヤーはピクセルキャッシュを使い、描画中のストロークだけDrawListで軽く描く。

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

    // ストローク変更の通知。
    // 指定レイヤーのBitmapを破棄し、次回draw時に点列から再構築する。
    void markDirty(int layerIndex);
    void markAllDirty();

    // ストロークをビットマップに焼く。
    // Step 1-4のペン確定時に呼ぶ想定。
    void bakeStroke(int layerIndex, const Stroke& stroke, float opacity);
    void eraseCircle(int layerIndex, float canvasX, float canvasY, float radius);
    void clearLayer(int layerIndex);

    // 毎フレーム呼ぶ描画メイン。
    // currentStroke はペン入力中のプレビュー。nullptrならプレビューなし。
    void draw(const Frame& frame,
              int activeLayerIndex,
              const Stroke* currentStroke,
              float currentStrokeOpacity,
              const CanvasView& view,
              ImVec2 areaMin,
              ImVec2 areaSize,
              ImDrawList* drawList);

    // オニオンスキン表示。
    // isPrevious=trueなら青系、falseなら赤系で表示する。
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
    std::unordered_map<int, CanvasBitmap> onionBitmaps_;
    std::unordered_map<int, std::uint64_t> onionRevisions_;

    CanvasBitmap& bitmapForLayer(int layerIndex);
    void rebuildLayerBitmapIfNeeded(int layerIndex, const Layer& layer);
    void rebuildOnionBitmapIfNeeded(const Frame& frame, int frameIndex);

    void drawBitmap(CanvasBitmap& bitmap,
                    float opacity,
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

    std::uint64_t frameRevisionHash(const Frame& frame) const;
};

} // namespace perapera
