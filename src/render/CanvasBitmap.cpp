// このファイルの役割:
// CanvasBitmapの実装。
// 1レイヤー = 1枚のSDL_Textureというルールを守り、ストローク確定時のみ
// ピクセルバッファへ描画し、dirty矩形だけをGPUへ転送する。

#include "render/CanvasBitmap.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace perapera {
namespace {

std::uint8_t toByte(float value)
{
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
}

float distance(float ax, float ay, float bx, float by)
{
    const float dx = bx - ax;
    const float dy = by - ay;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

CanvasBitmap::~CanvasBitmap()
{
    destroyTexture();
}

CanvasBitmap::CanvasBitmap(CanvasBitmap&& other) noexcept
{
    *this = std::move(other);
}

CanvasBitmap& CanvasBitmap::operator=(CanvasBitmap&& other) noexcept
{
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
    return *this;
}

void CanvasBitmap::resize(SDL_Renderer* renderer, int width, int height)
{
    const int safeWidth = std::max(0, width);
    const int safeHeight = std::max(0, height);

    destroyTexture();
    width_ = safeWidth;
    height_ = safeHeight;
    pixels_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 4U, 0U);

    dirty_ = false;
    dirtyX1_ = 0;
    dirtyY1_ = 0;
    dirtyX2_ = 0;
    dirtyY2_ = 0;

    if (width_ > 0 && height_ > 0 && renderer != nullptr) {
        createTexture(renderer);
        expandDirty(0, 0, width_, height_);
    }
}

void CanvasBitmap::bakeStroke(const Stroke& stroke, float opacity)
{
    if (width_ <= 0 || height_ <= 0 || stroke.points.empty()) {
        return;
    }

    const std::uint8_t r = toByte(stroke.color[0]);
    const std::uint8_t g = toByte(stroke.color[1]);
    const std::uint8_t b = toByte(stroke.color[2]);
    const std::uint8_t a = toByte(stroke.color[3] * opacity);

    const float baseRadius = std::max(0.5f, stroke.radiusPx);
    const float spacing = std::max(0.5f, baseRadius * 0.5f);

    if (stroke.points.size() == 1U) {
        const StrokePoint& point = stroke.points.front();
        const float radius = std::max(0.5f, baseRadius * std::clamp(point.pressure, 0.0f, 1.0f));
        stampCircle(point.x, point.y, radius, r, g, b, a);
        return;
    }

    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const float segmentLength = distance(previous.x, previous.y, current.x, current.y);
        const int steps = std::max(1, static_cast<int>(std::ceil(segmentLength / spacing)));

        for (int step = 0; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            const float x = previous.x + (current.x - previous.x) * t;
            const float y = previous.y + (current.y - previous.y) * t;
            const float pressure = previous.pressure + (current.pressure - previous.pressure) * t;
            const float radius = std::max(0.5f, baseRadius * std::clamp(pressure, 0.0f, 1.0f));
            stampCircle(x, y, radius, r, g, b, a);
        }
    }
}

void CanvasBitmap::eraseCircle(float cx, float cy, float radius)
{
    if (width_ <= 0 || height_ <= 0 || radius <= 0.0f) {
        return;
    }

    const int x0 = std::max(0, static_cast<int>(std::floor(cx - radius - 1.0f)));
    const int y0 = std::max(0, static_cast<int>(std::floor(cy - radius - 1.0f)));
    const int x1 = std::min(width_, static_cast<int>(std::ceil(cx + radius + 1.0f)));
    const int y1 = std::min(height_, static_cast<int>(std::ceil(cy + radius + 1.0f)));

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f) - cx;
            const float dy = (static_cast<float>(y) + 0.5f) - cy;
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius + 0.5f) {
                continue;
            }

            const float coverage = std::clamp(radius + 0.5f - dist, 0.0f, 1.0f);
            const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                        static_cast<std::size_t>(x)) * 4U;
            const float keepAlpha = 1.0f - coverage;
            pixels_[offset + 0U] = static_cast<std::uint8_t>(std::lround(static_cast<float>(pixels_[offset + 0U]) * keepAlpha));
            pixels_[offset + 1U] = static_cast<std::uint8_t>(std::lround(static_cast<float>(pixels_[offset + 1U]) * keepAlpha));
            pixels_[offset + 2U] = static_cast<std::uint8_t>(std::lround(static_cast<float>(pixels_[offset + 2U]) * keepAlpha));
            pixels_[offset + 3U] = static_cast<std::uint8_t>(std::lround(static_cast<float>(pixels_[offset + 3U]) * keepAlpha));
        }
    }

    expandDirty(x0, y0, x1, y1);
}

void CanvasBitmap::clear()
{
    std::fill(pixels_.begin(), pixels_.end(), static_cast<uint8_t>(0));
    if (width_ > 0 && height_ > 0) {
        expandDirty(0, 0, width_, height_);
    }
}

bool CanvasBitmap::uploadIfDirty(SDL_Renderer* renderer)
{
    if (!dirty_) {
        return true;
    }

    if (width_ <= 0 || height_ <= 0 || pixels_.empty()) {
        dirty_ = false;
        return true;
    }

    if (texture_ == nullptr && !createTexture(renderer)) {
        return false;
    }

    const int x0 = std::clamp(dirtyX1_, 0, width_);
    const int y0 = std::clamp(dirtyY1_, 0, height_);
    const int x1 = std::clamp(dirtyX2_, 0, width_);
    const int y1 = std::clamp(dirtyY2_, 0, height_);

    if (x0 >= x1 || y0 >= y1) {
        dirty_ = false;
        return true;
    }

    const SDL_Rect rect { x0, y0, x1 - x0, y1 - y0 };
    const std::size_t offset = (static_cast<std::size_t>(y0) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x0)) * 4U;
    const int pitch = width_ * 4;

    if (!SDL_UpdateTexture(texture_, &rect, pixels_.data() + offset, pitch)) {
        return false;
    }

    dirty_ = false;
    return true;
}

ImTextureID CanvasBitmap::imTextureID() const
{
    return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(texture_));
}

void CanvasBitmap::destroyTexture()
{
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
}

bool CanvasBitmap::createTexture(SDL_Renderer* renderer)
{
    if (renderer == nullptr || width_ <= 0 || height_ <= 0) {
        return false;
    }

    texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (texture_ == nullptr) {
        return false;
    }

    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    return true;
}

void CanvasBitmap::stampCircle(float cx, float cy, float radius,
                               std::uint8_t r, std::uint8_t g,
                               std::uint8_t b, std::uint8_t a)
{
    if (radius <= 0.0f || a == 0U) {
        return;
    }

    const int x0 = std::max(0, static_cast<int>(std::floor(cx - radius - 1.0f)));
    const int y0 = std::max(0, static_cast<int>(std::floor(cy - radius - 1.0f)));
    const int x1 = std::min(width_, static_cast<int>(std::ceil(cx + radius + 1.0f)));
    const int y1 = std::min(height_, static_cast<int>(std::ceil(cy + radius + 1.0f)));

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const float dx = (static_cast<float>(x) + 0.5f) - cx;
            const float dy = (static_cast<float>(y) + 0.5f) - cy;
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius + 0.5f) {
                continue;
            }

            const float coverage = std::clamp(radius + 0.5f - dist, 0.0f, 1.0f);
            const auto coveredAlpha = static_cast<std::uint8_t>(std::lround(static_cast<float>(a) * coverage));
            blendPixel(x, y, r, g, b, coveredAlpha);
        }
    }

    expandDirty(x0, y0, x1, y1);
}

void CanvasBitmap::blendPixel(int x, int y,
                              std::uint8_t r, std::uint8_t g,
                              std::uint8_t b, std::uint8_t a)
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_ || a == 0U) {
        return;
    }

    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x)) * 4U;

    const float srcA = static_cast<float>(a) / 255.0f;
    const float dstA = static_cast<float>(pixels_[offset + 3U]) / 255.0f;
    const float outA = srcA + dstA * (1.0f - srcA);

    if (outA <= std::numeric_limits<float>::epsilon()) {
        pixels_[offset + 0U] = 0U;
        pixels_[offset + 1U] = 0U;
        pixels_[offset + 2U] = 0U;
        pixels_[offset + 3U] = 0U;
        return;
    }

    const float srcR = static_cast<float>(r) / 255.0f;
    const float srcG = static_cast<float>(g) / 255.0f;
    const float srcB = static_cast<float>(b) / 255.0f;
    const float dstR = static_cast<float>(pixels_[offset + 0U]) / 255.0f;
    const float dstG = static_cast<float>(pixels_[offset + 1U]) / 255.0f;
    const float dstB = static_cast<float>(pixels_[offset + 2U]) / 255.0f;

    const float outR = (srcR * srcA + dstR * dstA * (1.0f - srcA)) / outA;
    const float outG = (srcG * srcA + dstG * dstA * (1.0f - srcA)) / outA;
    const float outB = (srcB * srcA + dstB * dstA * (1.0f - srcA)) / outA;

    pixels_[offset + 0U] = toByte(outR);
    pixels_[offset + 1U] = toByte(outG);
    pixels_[offset + 2U] = toByte(outB);
    pixels_[offset + 3U] = toByte(outA);
}

void CanvasBitmap::expandDirty(int x0, int y0, int x1, int y1)
{
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

} // namespace perapera
