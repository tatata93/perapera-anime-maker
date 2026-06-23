// src/render/CanvasBitmap.cpp
//
// CanvasBitmapは、ストローク点列を表示用ピクセルへ焼き込む場所。
// 毎フレーム全ストロークを描き直さず、変更された範囲だけをSDL_Textureへ
// アップロードすることで60fpsを守る。

#include "render/CanvasBitmap.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <utility>

namespace perapera {
namespace {

constexpr int kBytesPerPixel = 4;

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

std::uint8_t floatToByte(float value) {
    return static_cast<std::uint8_t>(std::lround(clamp01(value) * 255.0f));
}

int floorToInt(float value) {
    return static_cast<int>(std::floor(value));
}

int ceilToInt(float value) {
    return static_cast<int>(std::ceil(value));
}

} // namespace

CanvasBitmap::~CanvasBitmap() {
    destroyTexture();
}

CanvasBitmap::CanvasBitmap(CanvasBitmap&& other) noexcept {
    *this = std::move(other);
}

CanvasBitmap& CanvasBitmap::operator=(CanvasBitmap&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroyTexture();

    width_ = other.width_;
    height_ = other.height_;
    pixels_ = std::move(other.pixels_);
    texture_ = other.texture_;
    dirtyX1_ = other.dirtyX1_;
    dirtyY1_ = other.dirtyY1_;
    dirtyX2_ = other.dirtyX2_;
    dirtyY2_ = other.dirtyY2_;
    dirty_ = other.dirty_;

    other.width_ = 0;
    other.height_ = 0;
    other.texture_ = nullptr;
    other.dirty_ = false;
    other.dirtyX1_ = 0;
    other.dirtyY1_ = 0;
    other.dirtyX2_ = 0;
    other.dirtyY2_ = 0;

    return *this;
}

void CanvasBitmap::resize(SDL_Renderer* renderer, int width, int height) {
    width_ = std::max(0, width);
    height_ = std::max(0, height);

    destroyTexture();

    const std::size_t pixelCount = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    pixels_.assign(pixelCount * kBytesPerPixel, 0);

    if (renderer != nullptr && validSize()) {
        createTexture(renderer);
    }

    if (validSize()) {
        expandDirty(0, 0, width_, height_);
    } else {
        dirty_ = false;
    }
}

void CanvasBitmap::bakeStroke(const Stroke& stroke, float opacity) {
    if (!validSize() || stroke.points.empty()) {
        return;
    }

    const std::uint8_t r = floatToByte(stroke.color[0]);
    const std::uint8_t g = floatToByte(stroke.color[1]);
    const std::uint8_t b = floatToByte(stroke.color[2]);
    const std::uint8_t a = floatToByte(stroke.color[3] * opacity);

    if (a == 0) {
        return;
    }

    if (stroke.points.size() == 1) {
        const StrokePoint& point = stroke.points.front();
        const float radius = std::max(0.5f, stroke.radiusPx * clamp01(point.pressure));
        stampCircle(point.x, point.y, radius, r, g, b, a);
        return;
    }

    for (std::size_t i = 1; i < stroke.points.size(); ++i) {
        const StrokePoint& previous = stroke.points[i - 1];
        const StrokePoint& current = stroke.points[i];

        const float dx = current.x - previous.x;
        const float dy = current.y - previous.y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        const float previousRadius = std::max(0.5f, stroke.radiusPx * clamp01(previous.pressure));
        const float currentRadius = std::max(0.5f, stroke.radiusPx * clamp01(current.pressure));
        const float averageRadius = (previousRadius + currentRadius) * 0.5f;

        // 円スタンプの間隔を半径の半分にすると、線が途切れにくい。
        const float spacing = std::max(0.5f, averageRadius * 0.5f);
        const int stepCount = std::max(1, static_cast<int>(std::ceil(distance / spacing)));

        for (int step = 0; step <= stepCount; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(stepCount);
            const float x = previous.x + dx * t;
            const float y = previous.y + dy * t;
            const float radius = previousRadius + (currentRadius - previousRadius) * t;
            stampCircle(x, y, radius, r, g, b, a);
        }
    }
}

void CanvasBitmap::eraseCircle(float cx, float cy, float radius) {
    if (!validSize() || radius <= 0.0f) {
        return;
    }

    const int x0 = std::max(0, floorToInt(cx - radius - 1.0f));
    const int y0 = std::max(0, floorToInt(cy - radius - 1.0f));
    const int x1 = std::min(width_, ceilToInt(cx + radius + 1.0f));
    const int y1 = std::min(height_, ceilToInt(cy + radius + 1.0f));

    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    const float radiusSquared = radius * radius;

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float dx = px - cx;
            const float dy = py - cy;
            const float distanceSquared = dx * dx + dy * dy;

            if (distanceSquared <= radiusSquared) {
                const std::size_t offset =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x))
                    * kBytesPerPixel;
                pixels_[offset + 0] = 0;
                pixels_[offset + 1] = 0;
                pixels_[offset + 2] = 0;
                pixels_[offset + 3] = 0;
            }
        }
    }

    expandDirty(x0, y0, x1, y1);
}

void CanvasBitmap::clear() {
    std::fill(pixels_.begin(), pixels_.end(), 0);
    if (validSize()) {
        expandDirty(0, 0, width_, height_);
    }
}

bool CanvasBitmap::uploadIfDirty(SDL_Renderer* renderer) {
    if (!dirty_ || !validSize()) {
        return true;
    }

    if (texture_ == nullptr) {
        if (renderer == nullptr) {
            return false;
        }
        createTexture(renderer);
        if (texture_ == nullptr) {
            return false;
        }
    }

    const int x0 = std::clamp(dirtyX1_, 0, width_);
    const int y0 = std::clamp(dirtyY1_, 0, height_);
    const int x1 = std::clamp(dirtyX2_, 0, width_);
    const int y1 = std::clamp(dirtyY2_, 0, height_);

    if (x0 >= x1 || y0 >= y1) {
        dirty_ = false;
        return true;
    }

    const SDL_Rect dirtyRect{x0, y0, x1 - x0, y1 - y0};
    const std::size_t offset =
        (static_cast<std::size_t>(y0) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x0))
        * kBytesPerPixel;

    const bool ok = SDL_UpdateTexture(
        texture_,
        &dirtyRect,
        pixels_.data() + offset,
        width_ * kBytesPerPixel);

    if (ok) {
        dirty_ = false;
    }

    return ok;
}

ImTextureID CanvasBitmap::imTextureID() const {
    return static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(texture_));
}

void CanvasBitmap::createTexture(SDL_Renderer* renderer) {
    if (renderer == nullptr || !validSize()) {
        return;
    }

    destroyTexture();

    texture_ = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        width_,
        height_);

    if (texture_ != nullptr) {
        SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    }
}

void CanvasBitmap::destroyTexture() {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
}

void CanvasBitmap::stampCircle(float cx,
                               float cy,
                               float radius,
                               std::uint8_t r,
                               std::uint8_t g,
                               std::uint8_t b,
                               std::uint8_t a) {
    if (radius <= 0.0f || a == 0) {
        return;
    }

    const int x0 = std::max(0, floorToInt(cx - radius - 1.0f));
    const int y0 = std::max(0, floorToInt(cy - radius - 1.0f));
    const int x1 = std::min(width_, ceilToInt(cx + radius + 1.0f));
    const int y1 = std::min(height_, ceilToInt(cy + radius + 1.0f));

    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float dx = px - cx;
            const float dy = py - cy;
            const float distance = std::sqrt(dx * dx + dy * dy);

            // 半径境界の1pxだけを少し薄くして、最低限のジャギーを抑える。
            const float coverage = clamp01(radius + 0.5f - distance);
            if (coverage <= 0.0f) {
                continue;
            }

            const std::uint8_t coveredAlpha = static_cast<std::uint8_t>(std::lround(static_cast<float>(a) * coverage));
            blendPixel(x, y, r, g, b, coveredAlpha);
        }
    }

    expandDirty(x0, y0, x1, y1);
}

void CanvasBitmap::blendPixel(int x,
                              int y,
                              std::uint8_t r,
                              std::uint8_t g,
                              std::uint8_t b,
                              std::uint8_t a) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_ || a == 0) {
        return;
    }

    const std::size_t offset =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x))
        * kBytesPerPixel;

    const float srcA = static_cast<float>(a) / 255.0f;
    const float dstA = static_cast<float>(pixels_[offset + 3]) / 255.0f;
    const float outA = srcA + dstA * (1.0f - srcA);

    if (outA <= 0.0f) {
        pixels_[offset + 0] = 0;
        pixels_[offset + 1] = 0;
        pixels_[offset + 2] = 0;
        pixels_[offset + 3] = 0;
        return;
    }

    const float srcR = static_cast<float>(r) / 255.0f;
    const float srcG = static_cast<float>(g) / 255.0f;
    const float srcB = static_cast<float>(b) / 255.0f;
    const float dstR = static_cast<float>(pixels_[offset + 0]) / 255.0f;
    const float dstG = static_cast<float>(pixels_[offset + 1]) / 255.0f;
    const float dstB = static_cast<float>(pixels_[offset + 2]) / 255.0f;

    const float outR = (srcR * srcA + dstR * dstA * (1.0f - srcA)) / outA;
    const float outG = (srcG * srcA + dstG * dstA * (1.0f - srcA)) / outA;
    const float outB = (srcB * srcA + dstB * dstA * (1.0f - srcA)) / outA;

    pixels_[offset + 0] = floatToByte(outR);
    pixels_[offset + 1] = floatToByte(outG);
    pixels_[offset + 2] = floatToByte(outB);
    pixels_[offset + 3] = floatToByte(outA);
}

void CanvasBitmap::expandDirty(int x0, int y0, int x1, int y1) {
    x0 = std::clamp(x0, 0, width_);
    y0 = std::clamp(y0, 0, height_);
    x1 = std::clamp(x1, 0, width_);
    y1 = std::clamp(y1, 0, height_);

    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    if (!dirty_) {
        dirtyX1_ = x0;
        dirtyY1_ = y0;
        dirtyX2_ = x1;
        dirtyY2_ = y1;
        dirty_ = true;
        return;
    }

    dirtyX1_ = std::min(dirtyX1_, x0);
    dirtyY1_ = std::min(dirtyY1_, y0);
    dirtyX2_ = std::max(dirtyX2_, x1);
    dirtyY2_ = std::max(dirtyY2_, y1);
}

bool CanvasBitmap::validSize() const {
    return width_ > 0 && height_ > 0 && !pixels_.empty();
}

} // namespace perapera
