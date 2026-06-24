#pragma once

// このファイルの役割:
// 1レイヤーぶんのRGBA8ピクセルバッファとSDL_Textureを管理する。
// ストローク確定時だけCPU側ピクセルへ焼き込み、dirty矩形だけGPUへ転送する。

#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "core/Stroke.h"

namespace perapera {

class CanvasBitmap {
public:
    CanvasBitmap() = default;
    ~CanvasBitmap();

    CanvasBitmap(const CanvasBitmap&) = delete;
    CanvasBitmap& operator=(const CanvasBitmap&) = delete;

    CanvasBitmap(CanvasBitmap&& other) noexcept;
    CanvasBitmap& operator=(CanvasBitmap&& other) noexcept;

    void resize(SDL_Renderer* renderer, int width, int height);

    // ストロークをピクセルへ焼く。
    // 点列間をブラシ半径の約半分の間隔で補間して、円スタンプを押す。
    void bakeStroke(const Stroke& stroke, float opacity = 1.0f);

    // 円形消しゴム。指定範囲のアルファを0に近づける。
    void eraseCircle(float cx, float cy, float radius);

    // libmypaintなど外部ブラシエンジンから1つのdabを押すための入口。
    // stampCircleは内部用の硬い円スタンプなので、外部用には硬さと不透明度を受ける薄い口を用意する。
    void paintDab(float cx, float cy, float radius,
                  float r, float g, float b, float opacity, float hardness);

    // smudge系ブラシが色を拾うための平均色。現段階ではMyPaint接続の最低限実装に使う。
    void sampleAverageColor(float cx, float cy, float radius,
                            float& r, float& g, float& b, float& a) const;

    // ピクセルバッファを透明でクリアする。
    void clear();

    // dirty矩形だけSDL_Textureへアップロードする。
    // dirtyでなければ何もしないので、毎フレーム呼んでも軽い。
    bool uploadIfDirty(SDL_Renderer* renderer);

    ImTextureID imTextureID() const;
    int width() const { return width_; }
    int height() const { return height_; }
    bool hasTexture() const { return texture_ != nullptr; }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<std::uint8_t> pixels_;
    SDL_Texture* texture_ = nullptr;

    // dirty矩形は [x1, y1) 〜 [x2, y2) の半開区間で保持する。
    int dirtyX1_ = 0;
    int dirtyY1_ = 0;
    int dirtyX2_ = 0;
    int dirtyY2_ = 0;
    bool dirty_ = false;

    void destroyTexture();
    bool createTexture(SDL_Renderer* renderer);

    void stampCircle(float cx, float cy, float radius,
                     std::uint8_t r, std::uint8_t g,
                     std::uint8_t b, std::uint8_t a);
    void blendPixel(int x, int y,
                    std::uint8_t r, std::uint8_t g,
                    std::uint8_t b, std::uint8_t a);
    void expandDirty(int x0, int y0, int x1, int y1);
};

} // namespace perapera
