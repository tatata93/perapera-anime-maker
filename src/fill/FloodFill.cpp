// このファイルの役割:
// FloodFill.h の実装。
// Normal / ColorTrace レイヤーの線を壁としてラスタライズし、クリック位置から4方向に塗り広げる。
// Phase 1.5 Step 18e:
//   - MyPaintBrushEngineで描いた線も、実際のブラシ描画結果を壁として扱う。
//   - バケツ塗りを境界から内側へ縮めず、線の下へ少し潜り込ませて白い隙間を減らす。
//   - 結果はPaintストロークとして保存されるため、キャンバス表示・Undo・保存・PNG/MP4に同じ結果が反映される。
// Phase 1.5 Step 18g:
//   - ColorTrace線の半透明エッジも下塗り対象に含める。
//   - 塗りを壁ピクセルだけでなく、線周辺の下塗りバンドへ伸ばしてキャンバス上の白フチを減らす。
// Phase 1.5 Step 18h:
//   - BrushPanel側の境界下塗りpxクランプ漏れを前提に、1px白フチを消すための最終シールを追加。
//   - Paintスパンの端と行間を少し重ね、CanvasBitmapのアンチエイリアス由来の白点を減らす。

#include "fill/FloodFill.h"

#include "brush/MyPaintBrushEngine.h"
#include "render/CanvasBitmap.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace perapera::fill {
namespace {

std::size_t indexOf(int x, int y, int width)
{
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

bool isWallLayer(LayerType type)
{
    return type == LayerType::Normal || type == LayerType::ColorTrace;
}

void dilateMask(std::vector<std::uint8_t>& mask, int width, int height, int radius)
{
    if (radius <= 0 || mask.empty()) {
        return;
    }

    const std::vector<std::uint8_t> original = mask;
    const int safeRadius = std::max(0, radius);
    const int radiusSq = safeRadius * safeRadius;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (original[indexOf(x, y, width)] == 0U) {
                continue;
            }
            for (int dy = -safeRadius; dy <= safeRadius; ++dy) {
                for (int dx = -safeRadius; dx <= safeRadius; ++dx) {
                    if (dx * dx + dy * dy > radiusSq) {
                        continue;
                    }
                    const int nx = x + dx;
                    const int ny = y + dy;
                    if (nx >= 0 && ny >= 0 && nx < width && ny < height) {
                        mask[indexOf(nx, ny, width)] = 1U;
                    }
                }
            }
        }
    }
}

std::vector<std::uint8_t> dilatedMaskCopy(const std::vector<std::uint8_t>& mask, int width, int height, int radius)
{
    std::vector<std::uint8_t> result = mask;
    dilateMask(result, width, height, radius);
    return result;
}

int underpaintIterationRadius(const FloodFillSettings& settings)
{
    // UI上の0でも最低限の下塗りを行う。
    // ColorTrace線は彩色モードで半透明表示されるため、線の芯だけでなく
    // アンチエイリアス端の下までPaintが入っていないと白フチに見える。
    return std::clamp(settings.insetPx, 0, 16) + 6;
}

void bakeStrokeToBitmap(CanvasBitmap& bitmap, const Stroke& stroke, float layerOpacity)
{
    const float strokeOpacity = std::clamp(stroke.opacity, 0.05f, 1.0f);
    const float combinedOpacity = std::clamp(layerOpacity * strokeOpacity, 0.0f, 1.0f);
    if (combinedOpacity <= 0.0f || stroke.points.empty()) {
        return;
    }

    // MyPaint線をSimple円スタンプで近似すると、壁マスクが実際の線とずれてバケツが漏れる。
    // クリック時だけなので、ここではCanvasRendererと同じMyPaint再生を使って正確な壁を作る。
    if (stroke.brushEngine == StrokeBrushEngine::MyPaint) {
        MyPaintBrushEngine myPaintEngine;
        if (myPaintEngine.isLibraryAvailable()) {
            myPaintEngine.bakeStroke(bitmap, stroke, combinedOpacity);
            return;
        }
    }

    bitmap.bakeStroke(stroke, combinedOpacity);
}

void addBitmapAlphaToMask(std::vector<std::uint8_t>& mask,
                          const CanvasBitmap& bitmap,
                          int width,
                          int height,
                          std::uint8_t alphaThreshold)
{
    const std::vector<std::uint8_t>& pixels = bitmap.pixelsRgba();
    if (bitmap.width() != width || bitmap.height() != height || pixels.empty()) {
        return;
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t pixelOffset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                             static_cast<std::size_t>(x)) * 4U;
            if (pixels[pixelOffset + 3U] >= alphaThreshold) {
                mask[indexOf(x, y, width)] = 1U;
            }
        }
    }
}

struct WallMasks {
    // flood fill が越えてはいけない壁。小さな隙間閉じのため、硬めの線マスクを少し膨張する。
    std::vector<std::uint8_t> wall;

    // 塗り確定後にPaintを潜り込ませてもよい範囲。
    // ColorTraceの半透明エッジやアンチエイリアス端も含めるため、
    // 非常に薄い線アルファから作った線周辺バンドを使う。
    std::vector<std::uint8_t> underpaintBand;
};

WallMasks buildWallMasks(const Frame& frame,
                         int targetLayerIndex,
                         int width,
                         int height,
                         const FloodFillSettings& settings)
{
    WallMasks masks;
    masks.wall.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
    masks.underpaintBand.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);

    // tolerance が高いほど薄い線も壁として扱う。
    // MyPaintの柔らかい線も壁にしたいので、旧実装より低めのしきい値から始める。
    const auto wallThreshold = static_cast<std::uint8_t>(
        std::clamp(48 - settings.tolerance / 6, 4, 255));

    // 下塗り用は「壁として止めるか」ではなく「色を潜り込ませるべき線の存在」を見る。
    // そのため、ColorTraceやMyPaintの薄いアンチエイリアス端も拾う。
    constexpr std::uint8_t kUnderpaintLineThreshold = 1U;

    CanvasBitmap layerBitmap;
    layerBitmap.resize(nullptr, width, height);

    for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
        if (layerIndex == targetLayerIndex) {
            continue;
        }

        const Layer& layer = frame.layers[static_cast<std::size_t>(layerIndex)];
        if (!layer.visible || layer.opacity <= 0.0f || !isWallLayer(layer.type)) {
            continue;
        }

        layerBitmap.clear();
        for (const Stroke& stroke : layer.strokes) {
            bakeStrokeToBitmap(layerBitmap, stroke, layer.opacity);
        }

        addBitmapAlphaToMask(masks.wall, layerBitmap, width, height, wallThreshold);
        addBitmapAlphaToMask(masks.underpaintBand, layerBitmap, width, height, kUnderpaintLineThreshold);
    }

    // 隙間閉じは「閉じていない線の小穴」をふさぐための壁膨張。
    // MyPaintの柔らかい線は端が薄くなるので、最低1pxだけ補強する。
    const int wallGrowRadius = std::max(1, std::clamp(settings.gapClosePx, 0, 8));
    dilateMask(masks.wall, width, height, wallGrowRadius);

    // 下塗りバンドは線そのものだけでなく、線のすぐ内側/下側の白フチも含める。
    // FloodFillの本体はwallで止めているため、後処理でこのバンド内だけPaintを広げる。
    const int underpaintBandRadius = wallGrowRadius + underpaintIterationRadius(settings) + 2;
    masks.underpaintBand = dilatedMaskCopy(masks.underpaintBand, width, height, underpaintBandRadius);
    return masks;
}

int countMaskPixels(const std::vector<std::uint8_t>& mask)
{
    return static_cast<int>(std::count(mask.begin(), mask.end(), static_cast<std::uint8_t>(1U)));
}

void bleedFilledMaskIntoUnderpaintBand(std::vector<std::uint8_t>& filled,
                                         const std::vector<std::uint8_t>& underpaintBand,
                                         int width,
                                         int height,
                                         int radius)
{
    // 旧inset処理は塗りを境界から引っ込めるため、白い隙間の原因になった。
    // Step 18eでは壁ピクセルだけに塗りを伸ばしたが、ColorTraceの薄いエッジは
    // 壁判定から漏れることがある。ここでは線周辺の下塗りバンド内へ広げる。
    // Paintレイヤーは線の下地として使うため、半透明エッジの下まで色を潜り込ませる。
    const int safeRadius = std::max(0, radius);
    if (safeRadius <= 0 || filled.empty() || underpaintBand.empty()) {
        return;
    }

    constexpr int kNeighborCount = 8;
    constexpr int kNeighborDx[kNeighborCount] = {1, -1, 0, 0, 1, 1, -1, -1};
    constexpr int kNeighborDy[kNeighborCount] = {0, 0, 1, -1, 1, -1, 1, -1};

    for (int step = 0; step < safeRadius; ++step) {
        bool changed = false;
        std::vector<std::uint8_t> next = filled;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t center = indexOf(x, y, width);
                if (filled[center] != 0U || underpaintBand[center] == 0U) {
                    continue;
                }

                bool touchesFill = false;
                for (int neighborIndex = 0; neighborIndex < kNeighborCount; ++neighborIndex) {
                    const int nx = x + kNeighborDx[neighborIndex];
                    const int ny = y + kNeighborDy[neighborIndex];
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                        continue;
                    }
                    if (filled[indexOf(nx, ny, width)] != 0U) {
                        touchesFill = true;
                        break;
                    }
                }

                if (touchesFill) {
                    next[center] = 1U;
                    changed = true;
                }
            }
        }

        filled.swap(next);
        if (!changed) {
            break;
        }
    }
}


void sealOnePixelGapsForPaintRasterization(std::vector<std::uint8_t>& filled,
                                           const std::vector<std::uint8_t>& underpaintBand,
                                           int width,
                                           int height)
{
    // FloodFillのマスク上では埋まっていても、Paintは横スパンのブラシストロークとして保存される。
    // ブラシのアンチエイリアス・丸キャップ・整数化の都合で、ColorTrace線の直下に1pxだけ
    // 白点が残ることがあるため、線周辺バンド内の1px穴をPaint側の正データとして塞ぐ。
    if (filled.empty() || underpaintBand.empty()) {
        return;
    }

    std::vector<std::uint8_t> sealBand = underpaintBand;
    dilateMask(sealBand, width, height, 1);

    constexpr int kNeighborCount = 8;
    constexpr int kNeighborDx[kNeighborCount] = {1, -1, 0, 0, 1, 1, -1, -1};
    constexpr int kNeighborDy[kNeighborCount] = {0, 0, 1, -1, 1, -1, 1, -1};

    for (int pass = 0; pass < 2; ++pass) {
        bool changed = false;
        std::vector<std::uint8_t> next = filled;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::size_t center = indexOf(x, y, width);
                if (filled[center] != 0U || sealBand[center] == 0U) {
                    continue;
                }

                int neighborFillCount = 0;
                bool horizontalBridge = false;
                bool verticalBridge = false;
                bool diagonalBridgeA = false;
                bool diagonalBridgeB = false;

                for (int neighborIndex = 0; neighborIndex < kNeighborCount; ++neighborIndex) {
                    const int nx = x + kNeighborDx[neighborIndex];
                    const int ny = y + kNeighborDy[neighborIndex];
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                        continue;
                    }
                    if (filled[indexOf(nx, ny, width)] != 0U) {
                        ++neighborFillCount;
                    }
                }

                if (x > 0 && x + 1 < width) {
                    horizontalBridge = filled[indexOf(x - 1, y, width)] != 0U &&
                                       filled[indexOf(x + 1, y, width)] != 0U;
                }
                if (y > 0 && y + 1 < height) {
                    verticalBridge = filled[indexOf(x, y - 1, width)] != 0U &&
                                     filled[indexOf(x, y + 1, width)] != 0U;
                }
                if (x > 0 && x + 1 < width && y > 0 && y + 1 < height) {
                    diagonalBridgeA = filled[indexOf(x - 1, y - 1, width)] != 0U &&
                                      filled[indexOf(x + 1, y + 1, width)] != 0U;
                    diagonalBridgeB = filled[indexOf(x + 1, y - 1, width)] != 0U &&
                                      filled[indexOf(x - 1, y + 1, width)] != 0U;
                }

                if (neighborFillCount >= 2 || horizontalBridge || verticalBridge || diagonalBridgeA || diagonalBridgeB) {
                    next[center] = 1U;
                    changed = true;
                }
            }
        }

        filled.swap(next);
        if (!changed) {
            break;
        }
    }
}

Stroke makeSpanStroke(int y, int x0, int x1, const std::array<float, 4>& color)
{
    Stroke stroke;
    stroke.color = color;
    stroke.opacity = 1.0f;
    stroke.brushEngine = StrokeBrushEngine::Simple;

    // 細い横線では、斜め境界やアンチエイリアス部に白い点が残りやすい。
    // 2px弱の重なりを持つスパンにして、CanvasBitmapの丸キャップ/AA由来の1px穴を塞ぐ。
    // 塗り過ぎた部分はNormal/ColorTrace線の下に隠れるため、見た目上の白フチを優先する。
    stroke.radiusPx = 1.85f;

    const float fy = static_cast<float>(y) + 0.5f;
    if (x1 - x0 <= 1) {
        const float centerX = static_cast<float>(x0) + 0.5f;
        stroke.points.push_back(StrokePoint{centerX, fy, 1.0f});
    } else {
        const float startX = static_cast<float>(x0) - 0.85f;
        const float endX = static_cast<float>(x1) + 0.85f;
        stroke.points.push_back(StrokePoint{startX, fy, 1.0f});
        stroke.points.push_back(StrokePoint{endX, fy, 1.0f});
    }
    return stroke;
}

} // namespace

FloodFillResult makeFloodFillStrokes(const Frame& frame,
                                      int targetLayerIndex,
                                      int canvasWidth,
                                      int canvasHeight,
                                      int seedX,
                                      int seedY,
                                      const std::array<float, 4>& fillColor,
                                      const FloodFillSettings& settings)
{
    FloodFillResult result;
    const int width = std::max(0, canvasWidth);
    const int height = std::max(0, canvasHeight);
    if (width <= 0 || height <= 0) {
        result.message = "invalid canvas size";
        return result;
    }
    if (targetLayerIndex < 0 || targetLayerIndex >= static_cast<int>(frame.layers.size())) {
        result.message = "invalid target layer";
        return result;
    }
    if (seedX < 0 || seedY < 0 || seedX >= width || seedY >= height) {
        result.message = "clicked outside canvas";
        return result;
    }

    const WallMasks masks = buildWallMasks(frame, targetLayerIndex, width, height, settings);
    const std::vector<std::uint8_t>& wall = masks.wall;
    const std::size_t seedIndex = indexOf(seedX, seedY, width);
    if (wall[seedIndex] != 0U) {
        result.message = "clicked on wall line";
        return result;
    }

    std::vector<std::uint8_t> filled(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
    std::vector<int> queue;
    queue.reserve(4096U);
    queue.push_back(static_cast<int>(seedIndex));
    filled[seedIndex] = 1U;

    const int leakGuardPercent = std::clamp(settings.leakGuardPercent, 0, 100);
    const int maxAllowedPixels = (leakGuardPercent > 0)
        ? std::max(1, width * height * leakGuardPercent / 100)
        : 0;

    std::size_t head = 0U;
    while (head < queue.size()) {
        const int current = queue[head++];
        const int x = current % width;
        const int y = current / width;

        if (maxAllowedPixels > 0 && static_cast<int>(queue.size()) > maxAllowedPixels) {
            result.message = "fill leaked: area exceeded guard percent ("
                + std::to_string(leakGuardPercent) + "%)";
            return result;
        }

        const int neighbors[4][2] = {{x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}};
        for (const auto& neighbor : neighbors) {
            const int nx = neighbor[0];
            const int ny = neighbor[1];
            if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                continue;
            }
            const std::size_t nextIndex = indexOf(nx, ny, width);
            if (filled[nextIndex] != 0U || wall[nextIndex] != 0U) {
                continue;
            }
            filled[nextIndex] = 1U;
            queue.push_back(static_cast<int>(nextIndex));
        }
    }

    const int rawFilledPixelCount = static_cast<int>(queue.size());
    if (rawFilledPixelCount <= 0) {
        result.message = "nothing to fill";
        return result;
    }

    if (leakGuardPercent > 0 && maxAllowedPixels > 0 && rawFilledPixelCount >= maxAllowedPixels) {
        result.message = "fill leaked: area exceeded guard percent ("
            + std::to_string(leakGuardPercent) + "%)";
        return result;
    }

    // キャンバス表示にも白い隙間対策を反映するため、出力時だけでなく、
    // 保存されるPaintストローク自体をColorTrace線の半透明エッジ下まで伸ばす。
    const int underpaintRadius = underpaintIterationRadius(settings);
    bleedFilledMaskIntoUnderpaintBand(filled, masks.underpaintBand, width, height, underpaintRadius);
    sealOnePixelGapsForPaintRasterization(filled, masks.underpaintBand, width, height);

    result.filledPixelCount = countMaskPixels(filled);
    if (result.filledPixelCount <= 0) {
        result.message = "nothing to fill after boundary underpaint";
        return result;
    }

    for (int y = 0; y < height; ++y) {
        int x = 0;
        while (x < width) {
            while (x < width && filled[indexOf(x, y, width)] == 0U) {
                ++x;
            }
            const int start = x;
            while (x < width && filled[indexOf(x, y, width)] != 0U) {
                ++x;
            }
            if (start < x) {
                result.strokes.push_back(makeSpanStroke(y, start, x, fillColor));
            }
        }
    }

    result.success = !result.strokes.empty();
    if (result.success) {
        result.message = "filled with color-trace underpaint and 1px seal";
    } else {
        result.message = "no spans generated";
    }
    return result;
}

} // namespace perapera::fill
