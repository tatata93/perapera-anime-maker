// このファイルの役割:
// CanvasBitmapのRGBA8ピクセルバッファ、SDL_Texture、ストローク焼き込みを実装する。
// Phase 1.5 Step 19では、バケツ塗り専用のFillStrokeをマスクから直接ピクセルへ焼く。
// Phase 1.5 Step 19dでは、旧0/1 FillStrokeマスクも255相当として扱い、表示を安定させる。

#include "render/CanvasBitmap.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace perapera {
namespace {

std::uint8_t toByte(float value)
{
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

float pressureMappedValue(float baseValue, float pressure, float amount, float minimumScale)
{
    const float safePressure = std::clamp(pressure, 0.0f, 1.0f);
    const float safeAmount = std::clamp(amount, 0.0f, 1.0f);
    const float scale = (1.0f - safeAmount) + safeAmount * std::max(minimumScale, safePressure);
    return baseValue * scale;
}

void blendPixelAtOffset(std::vector<std::uint8_t>& pixels,
                        std::size_t offset,
                        std::uint8_t r,
                        std::uint8_t g,
                        std::uint8_t b,
                        std::uint8_t a)
{
    if (a == 0U) {
        return;
    }

    const std::uint8_t dstAByte = pixels[offset + 3U];
    if (a == 255U || dstAByte == 0U) {
        pixels[offset + 0U] = r;
        pixels[offset + 1U] = g;
        pixels[offset + 2U] = b;
        pixels[offset + 3U] = a;
        return;
    }

    const std::uint32_t srcA = static_cast<std::uint32_t>(a);
    const std::uint32_t dstA = static_cast<std::uint32_t>(dstAByte);
    const std::uint32_t dstFactor = (dstA * (255U - srcA) + 127U) / 255U;
    const std::uint32_t outA = srcA + dstFactor;
    if (outA == 0U) {
        return;
    }

    auto blendChannel = [&](std::uint8_t src, std::uint8_t dst) {
        const std::uint32_t value = static_cast<std::uint32_t>(src) * srcA +
                                    static_cast<std::uint32_t>(dst) * dstFactor;
        return static_cast<std::uint8_t>((value + outA / 2U) / outA);
    };

    pixels[offset + 0U] = blendChannel(r, pixels[offset + 0U]);
    pixels[offset + 1U] = blendChannel(g, pixels[offset + 1U]);
    pixels[offset + 2U] = blendChannel(b, pixels[offset + 2U]);
    pixels[offset + 3U] = static_cast<std::uint8_t>(outA);
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
    other.dirtyX1_ = 0;
    other.dirtyY1_ = 0;
    other.dirtyX2_ = 0;
    other.dirtyY2_ = 0;
    return *this;
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

    destroyTexture();
    texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, width_, height_);
    if (texture_ == nullptr) {
        return false;
    }
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    return true;
}

void CanvasBitmap::resize(SDL_Renderer* renderer, int width, int height)
{
    const int safeWidth = std::max(1, width);
    const int safeHeight = std::max(1, height);
    if (safeWidth == width_ && safeHeight == height_) {
        if (renderer != nullptr && texture_ == nullptr) {
            createTexture(renderer);
            expandDirty(0, 0, width_, height_);
        }
        return;
    }

    width_ = safeWidth;
    height_ = safeHeight;
    pixels_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 4U, 0U);
    destroyTexture();
    if (renderer != nullptr) {
        createTexture(renderer);
    }
    dirty_ = false;
    expandDirty(0, 0, width_, height_);
}

void CanvasBitmap::clear()
{
    std::fill(pixels_.begin(), pixels_.end(), static_cast<std::uint8_t>(0U));
    expandDirty(0, 0, width_, height_);
}

void CanvasBitmap::blendPixel(int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_ || a == 0U) {
        return;
    }

    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(x)) * 4U;

    const float srcA = static_cast<float>(a) / 255.0f;
    const float dstA = static_cast<float>(pixels_[offset + 3U]) / 255.0f;
    const float outA = srcA + dstA * (1.0f - srcA);
    if (outA <= 0.0f) {
        return;
    }

    const float srcR = static_cast<float>(r) / 255.0f;
    const float srcG = static_cast<float>(g) / 255.0f;
    const float srcB = static_cast<float>(b) / 255.0f;
    const float dstR = static_cast<float>(pixels_[offset + 0U]) / 255.0f;
    const float dstG = static_cast<float>(pixels_[offset + 1U]) / 255.0f;
    const float dstB = static_cast<float>(pixels_[offset + 2U]) / 255.0f;

    pixels_[offset + 0U] = toByte((srcR * srcA + dstR * dstA * (1.0f - srcA)) / outA);
    pixels_[offset + 1U] = toByte((srcG * srcA + dstG * dstA * (1.0f - srcA)) / outA);
    pixels_[offset + 2U] = toByte((srcB * srcA + dstB * dstA * (1.0f - srcA)) / outA);
    pixels_[offset + 3U] = toByte(outA);
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
            const float dx = static_cast<float>(x) + 0.5f - cx;
            const float dy = static_cast<float>(y) + 0.5f - cy;
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius + 0.5f) {
                continue;
            }
            const float coverage = std::clamp(radius + 0.5f - dist, 0.0f, 1.0f);
            blendPixel(x, y, r, g, b, toByte((static_cast<float>(a) / 255.0f) * coverage));
        }
    }

    expandDirty(x0, y0, x1, y1);
}

void CanvasBitmap::bakeStroke(const Stroke& stroke, float opacity)
{
    if (width_ <= 0 || height_ <= 0) {
        return;
    }

    // opacity は呼び出し側で layer opacity や stroke.opacity を合成した値として渡される。
    // ここで stroke.opacity を再度掛けると既存のSimple/MyPaintの見た目が変わるため、
    // CanvasBitmap側では stroke.color[3] だけを掛ける。
    const float combinedOpacity = std::clamp(opacity * stroke.color[3], 0.0f, 1.0f);
    if (combinedOpacity <= 0.0f) {
        return;
    }

    const std::uint8_t r = toByte(stroke.color[0]);
    const std::uint8_t g = toByte(stroke.color[1]);
    const std::uint8_t b = toByte(stroke.color[2]);

    if (stroke.brushEngine == StrokeBrushEngine::Fill) {
        const std::size_t expectedSize = static_cast<std::size_t>(std::max(0, stroke.bitmapWidth)) *
                                         static_cast<std::size_t>(std::max(0, stroke.bitmapHeight));
        if (stroke.bitmap.empty() || stroke.bitmapWidth <= 0 || stroke.bitmapHeight <= 0 ||
            expectedSize != stroke.bitmap.size()) {
            return;
        }

        const int dirtyX0 = std::clamp(stroke.bitmapX, 0, width_);
        const int dirtyY0 = std::clamp(stroke.bitmapY, 0, height_);
        const int dirtyX1 = std::clamp(stroke.bitmapX + stroke.bitmapWidth, 0, width_);
        const int dirtyY1 = std::clamp(stroke.bitmapY + stroke.bitmapHeight, 0, height_);
        if (dirtyX0 >= dirtyX1 || dirtyY0 >= dirtyY1) {
            return;
        }

        const std::uint8_t fullMaskAlpha = toByte(combinedOpacity);
        if (fullMaskAlpha == 0U) {
            return;
        }

        const int localX0 = dirtyX0 - stroke.bitmapX;
        const int localY0 = dirtyY0 - stroke.bitmapY;
        const int localX1 = dirtyX1 - stroke.bitmapX;
        const int localY1 = dirtyY1 - stroke.bitmapY;
        const std::size_t bitmapStride = static_cast<std::size_t>(stroke.bitmapWidth);
        const std::size_t canvasStride = static_cast<std::size_t>(width_);

        for (int localY = localY0; localY < localY1; ++localY) {
            std::size_t maskOffset = static_cast<std::size_t>(localY) * bitmapStride +
                                     static_cast<std::size_t>(localX0);
            std::size_t pixelOffset =
                (static_cast<std::size_t>(stroke.bitmapY + localY) * canvasStride +
                 static_cast<std::size_t>(stroke.bitmapX + localX0)) * 4U;

            for (int localX = localX0; localX < localX1; ++localX) {
                const std::uint8_t mask = stroke.bitmap[maskOffset++];
                if (mask == 0U) {
                    pixelOffset += 4U;
                    continue;
                }
                // FillStrokeの正規マスクは 0 / 255。
                // Step 19初期パッケージを適用済みのセッションでは 0 / 1 のマスクが残る可能性があるため、
                // 1も「塗る」として扱い、既存作業中データの表示を安定させる。
                const std::uint8_t sourceAlpha = (mask == 1U || mask == 255U)
                    ? fullMaskAlpha
                    : toByte(combinedOpacity * (static_cast<float>(mask) / 255.0f));
                blendPixelAtOffset(pixels_, pixelOffset, r, g, b, sourceAlpha);
                pixelOffset += 4U;
            }
        }
        expandDirty(dirtyX0, dirtyY0, dirtyX1, dirtyY1);
        return;
    }

    if (stroke.points.empty()) {
        return;
    }

    const float baseRadius = std::max(0.5f, stroke.radiusPx);
    const float spacingPx = std::max(0.5f, baseRadius * std::clamp(stroke.spacing, 0.05f, 1.0f));

    auto stampPoint = [&](const StrokePoint& point) {
        const float radius = std::max(0.25f, pressureMappedValue(baseRadius, point.pressure, stroke.pressureToSize, 0.20f));
        const float pointOpacity = pressureMappedValue(combinedOpacity, point.pressure, stroke.pressureToOpacity, 0.25f);
        stampCircle(point.x, point.y, radius, r, g, b, toByte(pointOpacity));
    };

    if (stroke.points.size() == 1U) {
        stampPoint(stroke.points.front());
        return;
    }

    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const float dx = current.x - previous.x;
        const float dy = current.y - previous.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        const int steps = std::max(1, static_cast<int>(std::ceil(distance / spacingPx)));

        for (int step = 0; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            StrokePoint sample;
            sample.x = previous.x + dx * t;
            sample.y = previous.y + dy * t;
            sample.pressure = previous.pressure + (current.pressure - previous.pressure) * t;
            stampPoint(sample);
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
            const float dx = static_cast<float>(x) + 0.5f - cx;
            const float dy = static_cast<float>(y) + 0.5f - cy;
            if (dx * dx + dy * dy > radius * radius) {
                continue;
            }
            const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                        static_cast<std::size_t>(x)) * 4U;
            pixels_[offset + 3U] = 0U;
        }
    }

    expandDirty(x0, y0, x1, y1);
}

void CanvasBitmap::paintDab(float cx, float cy, float radius,
                            float r, float g, float b, float opacity, float hardness)
{
    if (width_ <= 0 || height_ <= 0 || radius <= 0.0f || opacity <= 0.0f) {
        return;
    }

    const int x0 = std::max(0, static_cast<int>(std::floor(cx - radius - 1.0f)));
    const int y0 = std::max(0, static_cast<int>(std::floor(cy - radius - 1.0f)));
    const int x1 = std::min(width_, static_cast<int>(std::ceil(cx + radius + 1.0f)));
    const int y1 = std::min(height_, static_cast<int>(std::ceil(cy + radius + 1.0f)));

    const float safeHardness = std::clamp(hardness, 0.02f, 1.0f);
    const std::uint8_t rb = toByte(r);
    const std::uint8_t gb = toByte(g);
    const std::uint8_t bb = toByte(b);

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - cx;
            const float dy = static_cast<float>(y) + 0.5f - cy;
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius + 0.5f) {
                continue;
            }

            const float normalized = std::clamp(dist / std::max(0.5f, radius), 0.0f, 1.0f);
            float coverage = 1.0f;
            if (normalized > safeHardness) {
                const float t = (normalized - safeHardness) / std::max(0.001f, 1.0f - safeHardness);
                coverage = 1.0f - std::clamp(t, 0.0f, 1.0f);
            }
            blendPixel(x, y, rb, gb, bb, toByte(opacity * coverage));
        }
    }

    expandDirty(x0, y0, x1, y1);
}

void CanvasBitmap::sampleAverageColor(float cx, float cy, float radius,
                                      float& r, float& g, float& b, float& a) const
{
    r = 0.0f;
    g = 0.0f;
    b = 0.0f;
    a = 0.0f;
    if (width_ <= 0 || height_ <= 0 || pixels_.empty()) {
        return;
    }

    const int x0 = std::max(0, static_cast<int>(std::floor(cx - radius)));
    const int y0 = std::max(0, static_cast<int>(std::floor(cy - radius)));
    const int x1 = std::min(width_, static_cast<int>(std::ceil(cx + radius)));
    const int y1 = std::min(height_, static_cast<int>(std::ceil(cy + radius)));

    float sumR = 0.0f;
    float sumG = 0.0f;
    float sumB = 0.0f;
    float sumA = 0.0f;
    int count = 0;

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                        static_cast<std::size_t>(x)) * 4U;
            sumR += static_cast<float>(pixels_[offset + 0U]) / 255.0f;
            sumG += static_cast<float>(pixels_[offset + 1U]) / 255.0f;
            sumB += static_cast<float>(pixels_[offset + 2U]) / 255.0f;
            sumA += static_cast<float>(pixels_[offset + 3U]) / 255.0f;
            ++count;
        }
    }

    if (count <= 0) {
        return;
    }
    r = sumR / static_cast<float>(count);
    g = sumG / static_cast<float>(count);
    b = sumB / static_cast<float>(count);
    a = sumA / static_cast<float>(count);
}

bool CanvasBitmap::uploadIfDirty(SDL_Renderer* renderer)
{
    if (!dirty_) {
        return false;
    }
    if (texture_ == nullptr && renderer != nullptr) {
        createTexture(renderer);
    }
    if (texture_ == nullptr || pixels_.empty()) {
        dirty_ = false;
        return false;
    }

    SDL_Rect rect{dirtyX1_, dirtyY1_, dirtyX2_ - dirtyX1_, dirtyY2_ - dirtyY1_};
    const std::size_t offset = (static_cast<std::size_t>(dirtyY1_) * static_cast<std::size_t>(width_) +
                                static_cast<std::size_t>(dirtyX1_)) * 4U;
    SDL_UpdateTexture(texture_, &rect, pixels_.data() + offset, width_ * 4);
    dirty_ = false;
    return true;
}

ImTextureID CanvasBitmap::imTextureID() const
{
    return reinterpret_cast<ImTextureID>(texture_);
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
