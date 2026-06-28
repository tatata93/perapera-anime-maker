// このファイルの役割:
// PNG書き出しを実装する。
// 外部ライブラリ追加を避けるため、zlibの非圧縮deflateブロックを使う最小PNGエンコーダを内蔵する。
// Phase 1.5 Step 15: 書き出しモードごとに対象レイヤーと背景を切り替える。
// Phase 1.5 Step 18: Composite出力時だけColorTrace線を隣接Paint色で置換する。
// Phase 1.5 Step 18c〜18g: 一時的に出力時の白フチ補正を追加していた。
// Phase 1.5 Step 19: PaintはFillStrokeとして正データ化したため、出力時の白フチ補正は削除する。
// Phase 1.5 Step 19d: Paintレイヤー内はFillStrokeを先に焼き、手描きストロークを後から焼く。
// Phase 1.5 Step 19q: Composite出力でColorTrace線を単純スキップせず、近傍Paint色へ置換して白抜けを防ぐ。

#include "io/PngExporter.h"

#include "brush/MyPaintBrushEngine.h"
#include "render/CanvasBitmap.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace perapera {
namespace {

struct ImageRgba {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

struct PaintColorLookup {
    int width = 0;
    int height = 0;
    bool hasPaint = false;
    std::vector<std::uint8_t> distance;
    std::vector<std::uint8_t> colors;
};

struct ExportTaskResult {
    bool ok = false;
    std::string errorMessage;
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

void blendPixelAtOffset(ImageRgba& image,
                        std::size_t offset,
                        std::uint8_t r,
                        std::uint8_t g,
                        std::uint8_t b,
                        std::uint8_t a)
{
    if (a == 0U) {
        return;
    }

    const std::uint8_t dstAByte = image.pixels[offset + 3U];
    if (a == 255U || dstAByte == 0U) {
        image.pixels[offset + 0U] = r;
        image.pixels[offset + 1U] = g;
        image.pixels[offset + 2U] = b;
        image.pixels[offset + 3U] = a;
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

    image.pixels[offset + 0U] = blendChannel(r, image.pixels[offset + 0U]);
    image.pixels[offset + 1U] = blendChannel(g, image.pixels[offset + 1U]);
    image.pixels[offset + 2U] = blendChannel(b, image.pixels[offset + 2U]);
    image.pixels[offset + 3U] = static_cast<std::uint8_t>(outA);
}

std::array<std::uint8_t, 256U> buildOpacityTable(float opacity)
{
    std::array<std::uint8_t, 256U> table{};
    for (std::size_t alpha = 0; alpha < table.size(); ++alpha) {
        table[alpha] = toByte((static_cast<float>(alpha) / 255.0f) * opacity);
    }
    return table;
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

    const std::array<std::uint8_t, 256U> opacityTable = buildOpacityTable(safeLayerOpacity);
    for (std::size_t offset = 0; offset + 3U < pixels.size(); offset += 4U) {
        const std::uint8_t sourceA = pixels[offset + 3U];
        if (sourceA == 0U) {
            continue;
        }

        const std::uint8_t blendedA = opacityTable[sourceA];
        blendPixelAtOffset(image, offset, pixels[offset + 0U], pixels[offset + 1U], pixels[offset + 2U], blendedA);
    }
}

void blendImage(ImageRgba& image, const ImageRgba& source, float opacity)
{
    if (source.width != image.width || source.height != image.height || source.pixels.empty()) {
        return;
    }

    const float safeOpacity = std::clamp(opacity, 0.0f, 1.0f);
    if (safeOpacity <= 0.0f) {
        return;
    }

    const std::array<std::uint8_t, 256U> opacityTable = buildOpacityTable(safeOpacity);
    for (std::size_t offset = 0; offset + 3U < source.pixels.size(); offset += 4U) {
        const std::uint8_t sourceA = source.pixels[offset + 3U];
        if (sourceA == 0U) {
            continue;
        }
        const std::uint8_t blendedA = opacityTable[sourceA];
        blendPixelAtOffset(image,
                           offset,
                           source.pixels[offset + 0U],
                           source.pixels[offset + 1U],
                           source.pixels[offset + 2U],
                           blendedA);
    }
}

bool sampleNearestPaintColor(const ImageRgba& paintReference,
                             int centerX,
                             int centerY,
                             std::array<std::uint8_t, 4U>& color)
{
    // ColorTrace線は最終Compositeでは表示色を残さず、線の左右にあるPaint色へ置換する。
    // FillStrokeでは線芯を塗らないため、ColorTrace線上そのものはPaintが透明になりやすい。
    constexpr int kSampleRadius = 16;
    constexpr std::uint8_t kMinPaintAlpha = 8U;

    for (int radius = 0; radius <= kSampleRadius; ++radius) {
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
                if (paintReference.pixels[offset + 3U] < kMinPaintAlpha) {
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

PaintColorLookup buildPaintColorLookup(const ImageRgba& paintReference)
{
    PaintColorLookup lookup;
    lookup.width = paintReference.width;
    lookup.height = paintReference.height;
    if (paintReference.width <= 0 || paintReference.height <= 0 || paintReference.pixels.empty()) {
        return lookup;
    }

    constexpr std::uint8_t kNoPaintDistance = 255U;
    constexpr std::uint8_t kSampleRadius = 16U;
    constexpr std::uint8_t kMinPaintAlpha = 8U;
    const std::size_t pixelCount = static_cast<std::size_t>(paintReference.width) *
                                   static_cast<std::size_t>(paintReference.height);

    lookup.distance.assign(pixelCount, kNoPaintDistance);
    lookup.colors.assign(pixelCount * 4U, 0U);

    std::vector<int> queue;
    queue.reserve(pixelCount / 4U);
    for (std::size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
        const std::size_t offset = pixelIndex * 4U;
        if (paintReference.pixels[offset + 3U] < kMinPaintAlpha) {
            continue;
        }

        lookup.distance[pixelIndex] = 0U;
        lookup.colors[offset + 0U] = paintReference.pixels[offset + 0U];
        lookup.colors[offset + 1U] = paintReference.pixels[offset + 1U];
        lookup.colors[offset + 2U] = paintReference.pixels[offset + 2U];
        lookup.colors[offset + 3U] = paintReference.pixels[offset + 3U];
        queue.push_back(static_cast<int>(pixelIndex));
        lookup.hasPaint = true;
    }

    for (std::size_t head = 0; head < queue.size(); ++head) {
        const int pixelIndex = queue[head];
        const std::uint8_t distance = lookup.distance[static_cast<std::size_t>(pixelIndex)];
        if (distance >= kSampleRadius) {
            continue;
        }

        const int x = pixelIndex % paintReference.width;
        const int y = pixelIndex / paintReference.width;
        const std::size_t sourceOffset = static_cast<std::size_t>(pixelIndex) * 4U;
        for (int dy = -1; dy <= 1; ++dy) {
            const int nextY = y + dy;
            if (nextY < 0 || nextY >= paintReference.height) {
                continue;
            }
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) {
                    continue;
                }

                const int nextX = x + dx;
                if (nextX < 0 || nextX >= paintReference.width) {
                    continue;
                }

                const std::size_t nextIndex = static_cast<std::size_t>(nextY) *
                                              static_cast<std::size_t>(paintReference.width) +
                                              static_cast<std::size_t>(nextX);
                if (lookup.distance[nextIndex] != kNoPaintDistance) {
                    continue;
                }

                const std::size_t targetOffset = nextIndex * 4U;
                lookup.distance[nextIndex] = static_cast<std::uint8_t>(distance + 1U);
                lookup.colors[targetOffset + 0U] = lookup.colors[sourceOffset + 0U];
                lookup.colors[targetOffset + 1U] = lookup.colors[sourceOffset + 1U];
                lookup.colors[targetOffset + 2U] = lookup.colors[sourceOffset + 2U];
                lookup.colors[targetOffset + 3U] = lookup.colors[sourceOffset + 3U];
                queue.push_back(static_cast<int>(nextIndex));
            }
        }
    }

    return lookup;
}

bool nearestPaintColorFromLookup(const PaintColorLookup& lookup,
                                 std::size_t pixelIndex,
                                 std::array<std::uint8_t, 4U>& color)
{
    if (!lookup.hasPaint || pixelIndex >= lookup.distance.size() || lookup.distance[pixelIndex] == 255U) {
        return false;
    }

    const std::size_t offset = pixelIndex * 4U;
    color = {lookup.colors[offset + 0U],
             lookup.colors[offset + 1U],
             lookup.colors[offset + 2U],
             lookup.colors[offset + 3U]};
    return true;
}

void blendColorTraceAsPaintColor(ImageRgba& image,
                                 const CanvasBitmap& colorTraceBitmap,
                                 const PaintColorLookup& paintLookup,
                                 float layerOpacity)
{
    const std::vector<std::uint8_t>& pixels = colorTraceBitmap.pixelsRgba();
    if (colorTraceBitmap.width() != image.width || colorTraceBitmap.height() != image.height || pixels.empty()) {
        return;
    }

    const float safeLayerOpacity = std::clamp(layerOpacity, 0.0f, 1.0f);
    if (safeLayerOpacity <= 0.0f) {
        return;
    }

    const std::array<std::uint8_t, 256U> opacityTable = buildOpacityTable(safeLayerOpacity);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const std::size_t pixelIndex = static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                           static_cast<std::size_t>(x);
            const std::size_t offset = pixelIndex * 4U;
            const std::uint8_t traceAlpha = pixels[offset + 3U];
            if (traceAlpha == 0U) {
                continue;
            }

            std::array<std::uint8_t, 4U> replacementColor{};
            const bool hasPaintColor = nearestPaintColorFromLookup(paintLookup, pixelIndex, replacementColor);
            const std::uint8_t outR = hasPaintColor ? replacementColor[0U] : pixels[offset + 0U];
            const std::uint8_t outG = hasPaintColor ? replacementColor[1U] : pixels[offset + 1U];
            const std::uint8_t outB = hasPaintColor ? replacementColor[2U] : pixels[offset + 2U];
            const std::uint8_t outA = opacityTable[traceAlpha];
            blendPixelAtOffset(image, offset, outR, outG, outB, outA);
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

ImageRgba rasterizeFrame(const Frame& frame, int width, int height, ExportMode mode, bool includeBackground = true)
{
    ImageRgba image;
    image.width = std::max(1, width);
    image.height = std::max(1, height);
    image.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);

    if (includeBackground && usesWhiteBackground(mode)) {
        fillBackground(image, 255U, 255U, 255U, 255U);
    }

    std::vector<RenderedLayer> renderedLayers;
    renderedLayers.reserve(frame.layers.size());

    ImageRgba paintReference;
    if (mode == ExportMode::Composite) {
        paintReference.width = image.width;
        paintReference.height = image.height;
        paintReference.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);
    }

    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f || !shouldExportLayer(layer, mode)) {
            continue;
        }

        RenderedLayer rendered;
        rendered.type = layer.type;
        rendered.opacity = layer.opacity;
        rendered.bitmap.resize(nullptr, image.width, image.height);
        rendered.bitmap.clear();

        if (layer.type == LayerType::Paint) {
            // 表示側と同じく、Paint内ではFillStrokeを先に焼く。
            // その後に手描きストロークを重ねることで、同一Paintレイヤー内の線が
            // バケツ塗りで消える出力差を防ぐ。
            for (const Stroke& stroke : layer.strokes) {
                if (stroke.brushEngine == StrokeBrushEngine::Fill) {
                    rasterizeStrokeToBitmap(rendered.bitmap, stroke);
                }
            }
            for (const Stroke& stroke : layer.strokes) {
                if (stroke.brushEngine != StrokeBrushEngine::Fill) {
                    rasterizeStrokeToBitmap(rendered.bitmap, stroke);
                }
            }
        } else {
            for (const Stroke& stroke : layer.strokes) {
                rasterizeStrokeToBitmap(rendered.bitmap, stroke);
            }
        }

        if (mode == ExportMode::Composite && layer.type == LayerType::Paint) {
            blendLayerBitmap(paintReference, rendered.bitmap, layer.opacity);
        }

        renderedLayers.push_back(std::move(rendered));
    }

    PaintColorLookup paintLookup;
    if (mode == ExportMode::Composite) {
        const bool hasColorTraceLayer = std::any_of(renderedLayers.begin(), renderedLayers.end(), [](const RenderedLayer& rendered) {
            return rendered.type == LayerType::ColorTrace;
        });
        if (hasColorTraceLayer) {
            paintLookup = buildPaintColorLookup(paintReference);
        }
    }

    for (const RenderedLayer& rendered : renderedLayers) {
        if (mode == ExportMode::Composite && rendered.type == LayerType::ColorTrace) {
            // CompositeではColorTrace色をそのまま残さない。
            // ただし単純にスキップすると、線芯だけ白背景が露出するため、近傍Paint色で置換する。
            blendColorTraceAsPaintColor(image, rendered.bitmap, paintLookup, rendered.opacity);
            continue;
        }
        blendLayerBitmap(image, rendered.bitmap, rendered.opacity);
    }

    if (mode == ExportMode::Composite) {
        // PaintレイヤーがNormalより上に並んでいても、最終Compositeでは主線を最後に再合成する。
        // これは出力時の白フチ補正ではなく、通常の撮影合成順序を安定させるための処理。
        for (const RenderedLayer& rendered : renderedLayers) {
            if (rendered.type == LayerType::Normal) {
                blendLayerBitmap(image, rendered.bitmap, rendered.opacity);
            }
        }
    }

    return image;
}

constexpr std::array<std::uint32_t, 256U> makeCrc32Table()
{
    std::array<std::uint32_t, 256U> table{};
    for (std::uint32_t value = 0; value < table.size(); ++value) {
        std::uint32_t crc = value;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1U) ^ (0xedb88320U & (0U - (crc & 1U)));
        }
        table[value] = crc;
    }
    return table;
}

std::uint32_t crc32(const std::uint8_t* data, std::size_t size)
{
    static constexpr std::array<std::uint32_t, 256U> table = makeCrc32Table();
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t i = 0; i < size; ++i) {
        crc = table[(crc ^ data[i]) & 0xffU] ^ (crc >> 8U);
    }
    return crc ^ 0xffffffffU;
}

std::uint32_t adler32(const std::vector<std::uint8_t>& data)
{
    constexpr std::uint32_t mod = 65521U;
    constexpr std::size_t maxBlock = 5552U;
    std::uint32_t a = 1U;
    std::uint32_t b = 0U;
    std::size_t offset = 0;
    while (offset < data.size()) {
        const std::size_t end = std::min(data.size(), offset + maxBlock);
        for (; offset < end; ++offset) {
            a += data[offset];
            b += a;
        }
        a %= mod;
        b %= mod;
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
    const std::size_t blockCount = (raw.size() + 65534U) / 65535U;
    out.reserve(2U + raw.size() + blockCount * 5U + 4U);
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

ExportTaskResult writePngTask(ImageRgba image, std::filesystem::path outputPath)
{
    ExportTaskResult result;
    result.ok = writePng(image, outputPath, &result.errorMessage);
    return result;
}

const Frame* frameForCellAtExportIndex(const Cell& cell, int frameIndex)
{
    if (cell.frames.empty() || frameIndex < 0) {
        return nullptr;
    }

    const int safeFrameIndex = std::clamp(frameIndex, 0, static_cast<int>(cell.frames.size()) - 1);
    return &cell.frames[static_cast<std::size_t>(safeFrameIndex)];
}

bool waitForOldestExportTask(std::deque<std::future<ExportTaskResult>>& pendingTasks,
                             std::string* errorMessage)
{
    if (pendingTasks.empty()) {
        return true;
    }

    ExportTaskResult result = pendingTasks.front().get();
    pendingTasks.pop_front();
    if (!result.ok) {
        setError(errorMessage, result.errorMessage);
        return false;
    }
    return true;
}

ImageRgba rasterizeCellsFrame(const std::vector<const Cell*>& cells,
                              int frameIndex,
                              int width,
                              int height,
                              ExportMode mode)
{
    ImageRgba image;
    image.width = std::max(1, width);
    image.height = std::max(1, height);
    image.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);

    if (usesWhiteBackground(mode)) {
        fillBackground(image, 255U, 255U, 255U, 255U);
    }

    for (const Cell* cell : cells) {
        if (cell == nullptr || cell->opacity <= 0.0f) {
            continue;
        }
        const Frame* frame = frameForCellAtExportIndex(*cell, frameIndex);
        if (frame == nullptr) {
            continue;
        }
        ImageRgba cellImage = rasterizeFrame(*frame, width, height, mode, false);
        blendImage(image, cellImage, cell->opacity);
    }

    return image;
}

ImageRgba rasterizeCellFrameRefs(const std::vector<ExportCellFrameRef>& refs,
                                 int width,
                                 int height,
                                 ExportMode mode)
{
    ImageRgba image;
    image.width = std::max(1, width);
    image.height = std::max(1, height);
    image.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);

    if (usesWhiteBackground(mode)) {
        fillBackground(image, 255U, 255U, 255U, 255U);
    }

    for (const ExportCellFrameRef& ref : refs) {
        if (ref.cell == nullptr || ref.frame == nullptr || ref.cell->opacity <= 0.0f) {
            continue;
        }
        ImageRgba cellImage = rasterizeFrame(*ref.frame, width, height, mode, false);
        blendImage(image, cellImage, ref.cell->opacity);
    }

    return image;
}

int maxFrameCount(const std::vector<const Cell*>& cells)
{
    int frameCount = 0;
    for (const Cell* cell : cells) {
        if (cell == nullptr) {
            continue;
        }
        frameCount = std::max(frameCount, static_cast<int>(cell->frames.size()));
    }
    return frameCount;
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
    const std::vector<const Cell*> cells{&cell};
    return exportCellsFrameSequence(cells, outputFolder, width, height, mode, errorMessage);
}

bool PngExporter::exportCellsFrame(const std::vector<const Cell*>& cells,
                                   int frameIndex,
                                   const std::filesystem::path& outputPath,
                                   int width,
                                   int height,
                                   ExportMode mode,
                                   std::string* errorMessage)
{
    if (cells.empty()) {
        setError(errorMessage, "no cells to export");
        return false;
    }
    if (frameIndex < 0) {
        setError(errorMessage, "invalid frame index");
        return false;
    }

    return writePng(rasterizeCellsFrame(cells, frameIndex, width, height, mode), outputPath, errorMessage);
}

bool PngExporter::exportCellFrameRefs(const std::vector<ExportCellFrameRef>& refs,
                                      const std::filesystem::path& outputPath,
                                      int width,
                                      int height,
                                      ExportMode mode,
                                      std::string* errorMessage)
{
    return writePng(rasterizeCellFrameRefs(refs, width, height, mode), outputPath, errorMessage);
}

bool PngExporter::exportCellsFrameSequence(const std::vector<const Cell*>& cells,
                                           const std::filesystem::path& outputFolder,
                                           int width,
                                           int height,
                                           ExportMode mode,
                                           std::string* errorMessage)
{
    if (cells.empty()) {
        setError(errorMessage, "no cells to export");
        return false;
    }

    const int frameCount = maxFrameCount(cells);
    if (frameCount <= 0) {
        setError(errorMessage, "cell has no frames");
        return false;
    }

    const unsigned int hardwareThreads = std::max(1U, std::thread::hardware_concurrency());
    const std::size_t maxPendingWrites = static_cast<std::size_t>(std::clamp(hardwareThreads / 2U, 1U, 3U));
    std::deque<std::future<ExportTaskResult>> pendingWrites;

    for (int index = 0; index < frameCount; ++index) {
        const std::filesystem::path outputPath = sequencePath(outputFolder, index + 1);
        ImageRgba image = rasterizeCellsFrame(cells, index, width, height, mode);
        pendingWrites.push_back(std::async(std::launch::async, writePngTask, std::move(image), outputPath));
        if (pendingWrites.size() >= maxPendingWrites && !waitForOldestExportTask(pendingWrites, errorMessage)) {
            return false;
        }
    }

    while (!pendingWrites.empty()) {
        if (!waitForOldestExportTask(pendingWrites, errorMessage)) {
            return false;
        }
    }
    return true;
}

bool PngExporter::exportCellFrameRefSequence(const std::vector<std::vector<ExportCellFrameRef>>& frames,
                                             const std::filesystem::path& outputFolder,
                                             int width,
                                             int height,
                                             ExportMode mode,
                                             std::string* errorMessage)
{
    if (frames.empty()) {
        setError(errorMessage, "no timeline frames to export");
        return false;
    }

    const unsigned int hardwareThreads = std::max(1U, std::thread::hardware_concurrency());
    const std::size_t maxPendingWrites = static_cast<std::size_t>(std::clamp(hardwareThreads / 2U, 1U, 3U));
    std::deque<std::future<ExportTaskResult>> pendingWrites;

    for (int index = 0; index < static_cast<int>(frames.size()); ++index) {
        const std::filesystem::path outputPath = sequencePath(outputFolder, index + 1);
        ImageRgba image = rasterizeCellFrameRefs(frames[static_cast<std::size_t>(index)], width, height, mode);
        pendingWrites.push_back(std::async(std::launch::async, writePngTask, std::move(image), outputPath));
        if (pendingWrites.size() >= maxPendingWrites && !waitForOldestExportTask(pendingWrites, errorMessage)) {
            return false;
        }
    }

    while (!pendingWrites.empty()) {
        if (!waitForOldestExportTask(pendingWrites, errorMessage)) {
            return false;
        }
    }
    return true;
}

} // namespace perapera
