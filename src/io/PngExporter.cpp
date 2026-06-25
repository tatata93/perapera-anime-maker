// このファイルの役割:
// PNG書き出しを実装する。
// 外部ライブラリ追加を避けるため、zlibの非圧縮deflateブロックを使う最小PNGエンコーダを内蔵する。
// Phase 1.5 Step 15: 書き出しモードごとに対象レイヤーと背景を切り替える。
// Phase 1.5 Step 18: Composite出力時だけColorTrace線を隣接Paint色で置換する。
// Phase 1.5 Step 18c: ColorTrace色を残さず、線と塗りの白い隙間もPaint色で埋める。
// Phase 1.5 Step 18d: Paintを線マスク方向へ拡張し、輪郭線との白い隙間をさらに潰す。
// Phase 1.5 Step 18e: 既存Fillでも残る白い隙間を出力側でさらに強く吸収する。

#include "io/PngExporter.h"

#include "brush/MyPaintBrushEngine.h"
#include "render/CanvasBitmap.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace perapera {
namespace {

struct ImageRgba {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

std::uint8_t toByte(float value)
{
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

void fillBackground(ImageRgba& image, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    for (std::size_t offset = 0; offset + 3U < image.pixels.size(); offset += 4U) {
        image.pixels[offset + 0U] = r;
        image.pixels[offset + 1U] = g;
        image.pixels[offset + 2U] = b;
        image.pixels[offset + 3U] = a;
    }
}

void blendPixel(ImageRgba& image, int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    if (x < 0 || y < 0 || x >= image.width || y >= image.height || a == 0U) {
        return;
    }

    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                static_cast<std::size_t>(x)) * 4U;
    const float srcA = static_cast<float>(a) / 255.0f;
    const float dstA = static_cast<float>(image.pixels[offset + 3U]) / 255.0f;
    const float outA = srcA + dstA * (1.0f - srcA);
    if (outA <= 0.0f) {
        return;
    }

    const float srcR = static_cast<float>(r) / 255.0f;
    const float srcG = static_cast<float>(g) / 255.0f;
    const float srcB = static_cast<float>(b) / 255.0f;
    const float dstR = static_cast<float>(image.pixels[offset + 0U]) / 255.0f;
    const float dstG = static_cast<float>(image.pixels[offset + 1U]) / 255.0f;
    const float dstB = static_cast<float>(image.pixels[offset + 2U]) / 255.0f;

    image.pixels[offset + 0U] = toByte((srcR * srcA + dstR * dstA * (1.0f - srcA)) / outA);
    image.pixels[offset + 1U] = toByte((srcG * srcA + dstG * dstA * (1.0f - srcA)) / outA);
    image.pixels[offset + 2U] = toByte((srcB * srcA + dstB * dstA * (1.0f - srcA)) / outA);
    image.pixels[offset + 3U] = toByte(outA);
}

void rasterizeStrokeToBitmap(CanvasBitmap& bitmap, const Stroke& stroke)
{
    const float strokeOpacity = std::clamp(stroke.opacity, 0.05f, 1.0f);

    // PNG/MP4書き出しでも、表示キャッシュ再構築と同じくMyPaint経路を使う。
    // Simple近似だけだと、MyPaintで追加したブラシ設定が出力に反映されない。
    if (stroke.brushEngine == StrokeBrushEngine::MyPaint) {
        MyPaintBrushEngine myPaintEngine;
        if (myPaintEngine.isLibraryAvailable()) {
            myPaintEngine.bakeStroke(bitmap, stroke, strokeOpacity);
            return;
        }
    }

    bitmap.bakeStroke(stroke, strokeOpacity);
}

void blendLayerBitmap(ImageRgba& image, const CanvasBitmap& bitmap, float layerOpacity)
{
    const std::vector<std::uint8_t>& pixels = bitmap.pixelsRgba();
    if (bitmap.width() != image.width || bitmap.height() != image.height || pixels.empty()) {
        return;
    }

    const float safeLayerOpacity = std::clamp(layerOpacity, 0.0f, 1.0f);
    if (safeLayerOpacity <= 0.0f) {
        return;
    }

    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                        static_cast<std::size_t>(x)) * 4U;
            const std::uint8_t sourceA = pixels[offset + 3U];
            if (sourceA == 0U) {
                continue;
            }

            const std::uint8_t blendedA = toByte((static_cast<float>(sourceA) / 255.0f) * safeLayerOpacity);
            blendPixel(image, x, y, pixels[offset + 0U], pixels[offset + 1U], pixels[offset + 2U], blendedA);
        }
    }
}

bool sampleNearestPaintColor(const ImageRgba& paintReference,
                             int centerX,
                             int centerY,
                             int maxRadius,
                             std::array<std::uint8_t, 4U>& color)
{
    constexpr std::uint8_t kMinPaintAlpha = 8U;

    for (int radius = 0; radius <= maxRadius; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                    continue;
                }

                const int x = centerX + dx;
                const int y = centerY + dy;
                if (x < 0 || y < 0 || x >= paintReference.width || y >= paintReference.height) {
                    continue;
                }

                const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(paintReference.width) +
                                            static_cast<std::size_t>(x)) * 4U;
                if (paintReference.pixels[offset + 3U] <= kMinPaintAlpha) {
                    continue;
                }

                color = {paintReference.pixels[offset + 0U],
                         paintReference.pixels[offset + 1U],
                         paintReference.pixels[offset + 2U],
                         paintReference.pixels[offset + 3U]};
                return true;
            }
        }
    }

    return false;
}

bool isProtectedDarkLinePixel(const ImageRgba& image, int x, int y)
{
    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                static_cast<std::size_t>(x)) * 4U;
    if (image.pixels[offset + 3U] < 16U) {
        return false;
    }

    // Normal線画の黒い芯をPaint色で潰さないための保護。
    const int maxChannel = std::max({static_cast<int>(image.pixels[offset + 0U]),
                                     static_cast<int>(image.pixels[offset + 1U]),
                                     static_cast<int>(image.pixels[offset + 2U])});
    return maxChannel < 96;
}

bool isBackgroundGapPixel(const ImageRgba& image, int x, int y)
{
    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                static_cast<std::size_t>(x)) * 4U;
    if (image.pixels[offset + 3U] < 16U) {
        return true;
    }

    // Composite書き出しは白背景なので、白に近いピクセルを「塗りと線の隙間」とみなす。
    // 既にPaint色が入っている場所や黒線は触らない。
    return image.pixels[offset + 0U] >= 235U &&
           image.pixels[offset + 1U] >= 235U &&
           image.pixels[offset + 2U] >= 235U;
}

void writePaintReplacementPixel(ImageRgba& image,
                                int x,
                                int y,
                                const std::array<std::uint8_t, 4U>& color)
{
    if (x < 0 || y < 0 || x >= image.width || y >= image.height) {
        return;
    }
    if (isProtectedDarkLinePixel(image, x, y)) {
        return;
    }

    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                static_cast<std::size_t>(x)) * 4U;
    image.pixels[offset + 0U] = color[0U];
    image.pixels[offset + 1U] = color[1U];
    image.pixels[offset + 2U] = color[2U];
    image.pixels[offset + 3U] = std::max(image.pixels[offset + 3U], color[3U]);
}

bool isPaintBleedTargetPixel(const ImageRgba& image, int x, int y)
{
    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                static_cast<std::size_t>(x)) * 4U;

    // 黒い主線の芯は絶対に塗りで潰さない。
    // ただし、白背景と合成された薄いアンチエイリアス縁は、後で線を再合成する前提で
    // 下地だけPaint色に差し替える。これにより白いフチが残りにくくなる。
    const int maxChannel = std::max({static_cast<int>(image.pixels[offset + 0U]),
                                     static_cast<int>(image.pixels[offset + 1U]),
                                     static_cast<int>(image.pixels[offset + 2U])});
    if (image.pixels[offset + 3U] >= 16U && maxChannel < 128) {
        return false;
    }

    if (image.pixels[offset + 3U] < 16U) {
        return true;
    }

    return image.pixels[offset + 0U] >= 150U &&
           image.pixels[offset + 1U] >= 150U &&
           image.pixels[offset + 2U] >= 150U;
}

ImageRgba makeDilatedAlphaMask(const ImageRgba& sourceMask, int radius)
{
    ImageRgba result;
    result.width = sourceMask.width;
    result.height = sourceMask.height;
    result.pixels.assign(static_cast<std::size_t>(result.width) * static_cast<std::size_t>(result.height) * 4U, 0U);

    constexpr std::uint8_t kMinMaskAlpha = 8U;
    const int safeRadius = std::max(0, radius);
    const int radiusSq = safeRadius * safeRadius;

    for (int y = 0; y < sourceMask.height; ++y) {
        for (int x = 0; x < sourceMask.width; ++x) {
            const std::size_t sourceOffset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(sourceMask.width) +
                                             static_cast<std::size_t>(x)) * 4U;
            if (sourceMask.pixels[sourceOffset + 3U] <= kMinMaskAlpha) {
                continue;
            }

            const int x0 = std::max(0, x - safeRadius);
            const int y0 = std::max(0, y - safeRadius);
            const int x1 = std::min(sourceMask.width - 1, x + safeRadius);
            const int y1 = std::min(sourceMask.height - 1, y + safeRadius);
            for (int targetY = y0; targetY <= y1; ++targetY) {
                for (int targetX = x0; targetX <= x1; ++targetX) {
                    const int dx = targetX - x;
                    const int dy = targetY - y;
                    if (dx * dx + dy * dy > radiusSq) {
                        continue;
                    }
                    const std::size_t targetOffset = (static_cast<std::size_t>(targetY) * static_cast<std::size_t>(result.width) +
                                                      static_cast<std::size_t>(targetX)) * 4U;
                    result.pixels[targetOffset + 3U] = 255U;
                }
            }
        }
    }

    return result;
}

ImageRgba mergeAlphaMasks(const ImageRgba& a, const ImageRgba& b)
{
    ImageRgba result;
    result.width = a.width;
    result.height = a.height;
    result.pixels.assign(static_cast<std::size_t>(result.width) * static_cast<std::size_t>(result.height) * 4U, 0U);
    if (a.width != b.width || a.height != b.height) {
        return result;
    }

    for (std::size_t offset = 0; offset + 3U < result.pixels.size(); offset += 4U) {
        result.pixels[offset + 3U] = std::max(a.pixels[offset + 3U], b.pixels[offset + 3U]);
    }
    return result;
}

void bleedPaintIntoLineGaps(ImageRgba& image,
                            const ImageRgba& lineMask,
                            const ImageRgba& paintReference,
                            int lineBandRadius,
                            int maxBleedSteps)
{
    // Paint領域から白い隙間へ1pxずつ膨張させる。
    // 黒い主線の芯を通過しないため、輪郭の外側へ漏れにくい。
    constexpr std::uint8_t kMinPaintAlpha = 8U;
    constexpr int kNeighborCount = 8;
    constexpr int kNeighborDx[kNeighborCount] = {1, -1, 0, 0, 1, 1, -1, -1};
    constexpr int kNeighborDy[kNeighborCount] = {0, 0, 1, -1, 1, -1, 1, -1};

    if (lineMask.width != image.width || lineMask.height != image.height ||
        paintReference.width != image.width || paintReference.height != image.height) {
        return;
    }

    ImageRgba lineBand = makeDilatedAlphaMask(lineMask, lineBandRadius);
    std::vector<std::uint8_t> grownPaint = paintReference.pixels;

    const int safeSteps = std::max(0, maxBleedSteps);
    for (int step = 0; step < safeSteps; ++step) {
        bool changed = false;
        std::vector<std::uint8_t> nextPaint = grownPaint;

        for (int y = 0; y < image.height; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                            static_cast<std::size_t>(x)) * 4U;
                if (grownPaint[offset + 3U] > kMinPaintAlpha) {
                    continue;
                }
                if (lineBand.pixels[offset + 3U] == 0U) {
                    continue;
                }
                if (!isPaintBleedTargetPixel(image, x, y)) {
                    continue;
                }

                std::array<std::uint8_t, 4U> neighborColor{};
                bool foundNeighborPaint = false;
                for (int neighborIndex = 0; neighborIndex < kNeighborCount; ++neighborIndex) {
                    const int nx = x + kNeighborDx[neighborIndex];
                    const int ny = y + kNeighborDy[neighborIndex];
                    if (nx < 0 || ny < 0 || nx >= image.width || ny >= image.height) {
                        continue;
                    }

                    const std::size_t neighborOffset = (static_cast<std::size_t>(ny) * static_cast<std::size_t>(image.width) +
                                                        static_cast<std::size_t>(nx)) * 4U;
                    if (grownPaint[neighborOffset + 3U] <= kMinPaintAlpha) {
                        continue;
                    }

                    neighborColor = {grownPaint[neighborOffset + 0U],
                                     grownPaint[neighborOffset + 1U],
                                     grownPaint[neighborOffset + 2U],
                                     grownPaint[neighborOffset + 3U]};
                    foundNeighborPaint = true;
                    break;
                }

                if (!foundNeighborPaint) {
                    continue;
                }

                nextPaint[offset + 0U] = neighborColor[0U];
                nextPaint[offset + 1U] = neighborColor[1U];
                nextPaint[offset + 2U] = neighborColor[2U];
                nextPaint[offset + 3U] = neighborColor[3U];
                writePaintReplacementPixel(image, x, y, neighborColor);
                changed = true;
            }
        }

        grownPaint.swap(nextPaint);
        if (!changed) {
            break;
        }
    }
}

void accumulateLineMask(ImageRgba& mask, const CanvasBitmap& bitmap, float layerOpacity)
{
    const std::vector<std::uint8_t>& pixels = bitmap.pixelsRgba();
    if (bitmap.width() != mask.width || bitmap.height() != mask.height || pixels.empty()) {
        return;
    }

    const float safeLayerOpacity = std::clamp(layerOpacity, 0.0f, 1.0f);
    if (safeLayerOpacity <= 0.0f) {
        return;
    }

    for (int y = 0; y < mask.height; ++y) {
        for (int x = 0; x < mask.width; ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(mask.width) +
                                        static_cast<std::size_t>(x)) * 4U;
            const std::uint8_t alpha = toByte((static_cast<float>(pixels[offset + 3U]) / 255.0f) * safeLayerOpacity);
            if (alpha == 0U) {
                continue;
            }
            mask.pixels[offset + 3U] = std::max(mask.pixels[offset + 3U], alpha);
        }
    }
}

void closePaintGapsNearLineMask(ImageRgba& image,
                                const ImageRgba& lineMask,
                                const ImageRgba& paintReference,
                                int fillRadius,
                                int paintSearchRadius)
{
    // Composite出力時だけ、線の周囲にある白い隙間を近傍Paint色で埋める。
    // Normal線の黒は保護し、ColorTrace線は描かずにPaint色へ吸収する。
    constexpr std::uint8_t kMinLineAlpha = 8U;

    for (int y = 0; y < lineMask.height; ++y) {
        for (int x = 0; x < lineMask.width; ++x) {
            const std::size_t lineOffset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(lineMask.width) +
                                            static_cast<std::size_t>(x)) * 4U;
            if (lineMask.pixels[lineOffset + 3U] <= kMinLineAlpha) {
                continue;
            }

            for (int dy = -fillRadius; dy <= fillRadius; ++dy) {
                for (int dx = -fillRadius; dx <= fillRadius; ++dx) {
                    if (dx * dx + dy * dy > fillRadius * fillRadius) {
                        continue;
                    }

                    const int targetX = x + dx;
                    const int targetY = y + dy;
                    if (targetX < 0 || targetY < 0 || targetX >= image.width || targetY >= image.height) {
                        continue;
                    }
                    if (!isBackgroundGapPixel(image, targetX, targetY)) {
                        continue;
                    }

                    std::array<std::uint8_t, 4U> replacementColor{};
                    if (!sampleNearestPaintColor(paintReference, targetX, targetY, paintSearchRadius, replacementColor)) {
                        continue;
                    }

                    writePaintReplacementPixel(image, targetX, targetY, replacementColor);
                }
            }
        }
    }
}

bool usesWhiteBackground(ExportMode mode)
{
    return mode != ExportMode::LineOnly;
}

bool shouldExportLayer(const Layer& layer, ExportMode mode)
{
    switch (mode) {
    case ExportMode::Composite:
        return true;
    case ExportMode::LineTest:
    case ExportMode::LineOnly:
        return layer.type == LayerType::Normal;
    case ExportMode::ColorTrace:
        return layer.type == LayerType::Normal || layer.type == LayerType::ColorTrace;
    }
    return true;
}

struct RenderedLayer {
    LayerType type = LayerType::Normal;
    float opacity = 1.0f;
    CanvasBitmap bitmap;
};

ImageRgba rasterizeFrame(const Frame& frame, int width, int height, ExportMode mode)
{
    ImageRgba image;
    image.width = std::max(1, width);
    image.height = std::max(1, height);
    image.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);

    if (usesWhiteBackground(mode)) {
        fillBackground(image, 255U, 255U, 255U, 255U);
    }

    std::vector<RenderedLayer> renderedLayers;
    renderedLayers.reserve(frame.layers.size());

    ImageRgba paintReference;
    paintReference.width = image.width;
    paintReference.height = image.height;
    paintReference.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);

    ImageRgba normalLineMask;
    normalLineMask.width = image.width;
    normalLineMask.height = image.height;
    normalLineMask.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);

    ImageRgba colorTraceMask;
    colorTraceMask.width = image.width;
    colorTraceMask.height = image.height;
    colorTraceMask.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);

    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f || !shouldExportLayer(layer, mode)) {
            continue;
        }

        RenderedLayer rendered;
        rendered.type = layer.type;
        rendered.opacity = layer.opacity;
        rendered.bitmap.resize(nullptr, image.width, image.height);
        rendered.bitmap.clear();

        for (const Stroke& stroke : layer.strokes) {
            rasterizeStrokeToBitmap(rendered.bitmap, stroke);
        }

        if (mode == ExportMode::Composite && layer.type == LayerType::Paint) {
            // ColorTrace置換と白い隙間埋め用の参照色。
            // 出力順に関係なくPaint色を拾えるよう、先に合成しておく。
            blendLayerBitmap(paintReference, rendered.bitmap, layer.opacity);
        }
        if (mode == ExportMode::Composite && layer.type == LayerType::Normal) {
            accumulateLineMask(normalLineMask, rendered.bitmap, layer.opacity);
        }
        if (mode == ExportMode::Composite && layer.type == LayerType::ColorTrace) {
            accumulateLineMask(colorTraceMask, rendered.bitmap, layer.opacity);
        }

        renderedLayers.push_back(std::move(rendered));
    }

    for (const RenderedLayer& rendered : renderedLayers) {
        if (mode == ExportMode::Composite && rendered.type == LayerType::ColorTrace) {
            // CompositeではColorTraceの赤・青・黄緑をそのまま描かない。
            // 後段のclosePaintGapsNearLineMaskでPaint色に吸収する。
            continue;
        }
        blendLayerBitmap(image, rendered.bitmap, rendered.opacity);
    }

    if (mode == ExportMode::Composite) {
        const ImageRgba combinedLineMask = mergeAlphaMasks(normalLineMask, colorTraceMask);

        // Paint領域を線方向へ膨張させて、バケツ塗りと輪郭線の白い隙間を埋める。
        // その後にNormal線を再合成し、黒い輪郭とアンチエイリアスをPaint色の上に戻す。
        bleedPaintIntoLineGaps(image, combinedLineMask, paintReference, 24, 32);

        // ColorTrace線は最終出力で色を残さず、Paint色へ吸収する。
        // 残った細い白点を拾うため、従来の近傍サンプリングも最後に薄くかける。
        closePaintGapsNearLineMask(image, combinedLineMask, paintReference, 12, 40);

        for (const RenderedLayer& rendered : renderedLayers) {
            if (rendered.type == LayerType::Normal) {
                blendLayerBitmap(image, rendered.bitmap, rendered.opacity);
            }
        }
    }

    return image;
}

std::uint32_t crc32(const std::uint8_t* data, std::size_t size)
{
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1U) ^ (0xedb88320U & (0U - (crc & 1U)));
        }
    }
    return crc ^ 0xffffffffU;
}

std::uint32_t adler32(const std::vector<std::uint8_t>& data)
{
    constexpr std::uint32_t mod = 65521U;
    std::uint32_t a = 1U;
    std::uint32_t b = 0U;
    for (std::uint8_t value : data) {
        a = (a + value) % mod;
        b = (b + a) % mod;
    }
    return (b << 16U) | a;
}

void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    out.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void appendChunk(std::vector<std::uint8_t>& png, const char type[4], const std::vector<std::uint8_t>& data)
{
    appendU32(png, static_cast<std::uint32_t>(data.size()));
    const std::size_t typeOffset = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    const std::uint32_t crc = crc32(png.data() + typeOffset, png.size() - typeOffset);
    appendU32(png, crc);
}

std::vector<std::uint8_t> zlibStore(const std::vector<std::uint8_t>& raw)
{
    std::vector<std::uint8_t> out;
    out.push_back(0x78U);
    out.push_back(0x01U);

    std::size_t offset = 0;
    while (offset < raw.size()) {
        const std::size_t remaining = raw.size() - offset;
        const std::uint16_t blockSize = static_cast<std::uint16_t>(std::min<std::size_t>(remaining, 65535U));
        const bool finalBlock = offset + blockSize >= raw.size();
        out.push_back(finalBlock ? 1U : 0U);
        out.push_back(static_cast<std::uint8_t>(blockSize & 0xffU));
        out.push_back(static_cast<std::uint8_t>((blockSize >> 8U) & 0xffU));
        const std::uint16_t inverse = static_cast<std::uint16_t>(~blockSize);
        out.push_back(static_cast<std::uint8_t>(inverse & 0xffU));
        out.push_back(static_cast<std::uint8_t>((inverse >> 8U) & 0xffU));
        out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset), raw.begin() + static_cast<std::ptrdiff_t>(offset + blockSize));
        offset += blockSize;
    }

    appendU32(out, adler32(raw));
    return out;
}

bool writePng(const ImageRgba& image, const std::filesystem::path& outputPath, std::string* errorMessage)
{
    std::error_code errorCode;
    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path(), errorCode);
        if (errorCode) {
            setError(errorMessage, errorCode.message());
            return false;
        }
    }

    std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<std::size_t>(image.height) * (static_cast<std::size_t>(image.width) * 4U + 1U));
    for (int y = 0; y < image.height; ++y) {
        raw.push_back(0U);
        const std::size_t rowOffset = static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) * 4U;
        raw.insert(raw.end(), image.pixels.begin() + static_cast<std::ptrdiff_t>(rowOffset),
                   image.pixels.begin() + static_cast<std::ptrdiff_t>(rowOffset + static_cast<std::size_t>(image.width) * 4U));
    }

    std::vector<std::uint8_t> png = {0x89U, 'P', 'N', 'G', 0x0dU, 0x0aU, 0x1aU, 0x0aU};
    std::vector<std::uint8_t> ihdr;
    appendU32(ihdr, static_cast<std::uint32_t>(image.width));
    appendU32(ihdr, static_cast<std::uint32_t>(image.height));
    ihdr.push_back(8U);
    ihdr.push_back(6U);
    ihdr.push_back(0U);
    ihdr.push_back(0U);
    ihdr.push_back(0U);
    appendChunk(png, "IHDR", ihdr);
    appendChunk(png, "IDAT", zlibStore(raw));
    appendChunk(png, "IEND", {});

    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        setError(errorMessage, "failed to open output file");
        return false;
    }
    file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return true;
}

std::filesystem::path sequencePath(const std::filesystem::path& folder, int index)
{
    std::ostringstream name;
    name << "frame_" << std::setw(3) << std::setfill('0') << index << ".png";
    return folder / name.str();
}

} // namespace

const char* exportModeToString(ExportMode mode)
{
    switch (mode) {
    case ExportMode::Composite:
        return "Composite";
    case ExportMode::LineTest:
        return "LineTest";
    case ExportMode::ColorTrace:
        return "ColorTrace";
    case ExportMode::LineOnly:
        return "LineOnly";
    }
    return "Composite";
}

const char* exportModeDisplayName(ExportMode mode)
{
    switch (mode) {
    case ExportMode::Composite:
        return "通常（全レイヤー合成）";
    case ExportMode::LineTest:
        return "ラインテスト（線画のみ・白背景）";
    case ExportMode::ColorTrace:
        return "色トレスアニメ（線画＋色トレス線・白背景）";
    case ExportMode::LineOnly:
        return "線画素材（線画のみ・背景透明）";
    }
    return "通常（全レイヤー合成）";
}

bool PngExporter::exportFrame(const Frame& frame,
                              const std::filesystem::path& outputPath,
                              int width,
                              int height,
                              ExportMode mode,
                              std::string* errorMessage)
{
    return writePng(rasterizeFrame(frame, width, height, mode), outputPath, errorMessage);
}

bool PngExporter::exportFrameSequence(const Cell& cell,
                                      const std::filesystem::path& outputFolder,
                                      int width,
                                      int height,
                                      ExportMode mode,
                                      std::string* errorMessage)
{
    if (cell.frames.empty()) {
        setError(errorMessage, "cell has no frames");
        return false;
    }

    for (std::size_t index = 0; index < cell.frames.size(); ++index) {
        const std::filesystem::path outputPath = sequencePath(outputFolder, static_cast<int>(index) + 1);
        if (!exportFrame(cell.frames[index], outputPath, width, height, mode, errorMessage)) {
            return false;
        }
    }
    return true;
}

} // namespace perapera
