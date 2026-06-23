#pragma once

// src/render/CanvasBitmap.h
//
// 1レイヤー分のピクセルバッファとSDL_Textureを管理する。
// 正データはcore層のStroke点列であり、CanvasBitmapは表示を軽くするための
// 1層だけのキャッシュとして使う。

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

    // キャンバスサイズを変更し、RGBA8ピクセルバッファを透明で初期化する。
    // rendererがnullptrの場合はCPU側だけを用意し、後でuploadIfDirty時にTextureを作る。
    void resize(SDL_Renderer* renderer, int width, int height);

    // ストロークをピクセルに焼く。
    // ペン確定時のみ呼ぶ想定で、毎フレーム呼ばない。
    void bakeStroke(const Stroke& stroke, float opacity = 1.0f);

    // 円形消しゴム。指定円内のアルファを0へ近づける。
    void eraseCircle(float cx, float cy, float radius);

    // 全体を透明にして、全範囲をdirtyにする。
    void clear();

    // dirty矩形だけSDL_Textureへアップロードする。
    // dirtyでなければ何もしないので、毎フレーム呼んでも軽い。
    bool uploadIfDirty(SDL_Renderer* renderer);

    ImTextureID imTextureID() const;
    int width() const { return width_; }
    int height() const { return height_; }
    bool hasTexture() const { return texture_ != nullptr; }
    bool dirty() const { return dirty_; }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<std::uint8_t> pixels_; // RGBA8。1ピクセル4バイト。
    SDL_Texture* texture_ = nullptr;

    int dirtyX1_ = 0;
    int dirtyY1_ = 0;
    int dirtyX2_ = 0;
    int dirtyY2_ = 0;
    bool dirty_ = false;

    void createTexture(SDL_Renderer* renderer);
    void destroyTexture();

    void stampCircle(float cx,
                     float cy,
                     float radius,
                     std::uint8_t r,
                     std::uint8_t g,
                     std::uint8_t b,
                     std::uint8_t a);

    void blendPixel(int x,
                    int y,
                    std::uint8_t r,
                    std::uint8_t g,
                    std::uint8_t b,
                    std::uint8_t a);

    void expandDirty(int x0, int y0, int x1, int y1);
    bool validSize() const;
};

} // namespace perapera
