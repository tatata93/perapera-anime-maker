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
// Phase 1.5 Step 18i:
//   - MyPaint線は低アルファまで壁として拾い、線画と塗りの隙間を減らす。
//   - 細いSimple線では下塗りが線の外側へ抜けないよう、外側到達マスクで制限する。
//   - Paintスパンストロークを太らせすぎない設定に戻し、細線の外側へ色が見える事故を減らす。

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
    // ただしStep 18hのように大きく足しすぎると、細いSimple線の外側まで
    // Paintが見えることがある。Step 18iでは外側到達マスクで越境を止めるため、
    // ここでは「線の半透明エッジを覆う」程度の半径に抑える。
    return std::clamp(settings.insetPx, 0, 16) + 2;
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
    // flood fill が越えてはいけない壁。小さな隙間閉じのため、線マスクを少し膨張する。
    std::vector<std::uint8_t> wall;

    // 塗り確定後にPaintを潜り込ませてもよい範囲。
    // ColorTrace/MyPaintの半透明エッジやアンチエイリアス端も含める。
    std::vector<std::uint8_t> underpaintBand;

    // キャンバス外側から到達できる領域を求めるための壁。
    // 下塗りはこの外側到達領域には入れない。これで細いSimple線の外側へ
    // Paintが抜ける事故を抑える。
    std::vector<std::uint8_t> exteriorBarrier;
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
    masks.exteriorBarrier.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);

    // tolerance が高いほど薄い線も壁として扱う。
    // MyPaintは線端や筆圧の弱い部分が低アルファになりやすいため、
    // Step 18hより低いしきい値で壁に入れる。
    // これで「追加したブラシの線画だけバケツとの間に隙間が残る」問題を抑える。
    const auto wallThreshold = static_cast<std::uint8_t>(
        std::clamp(24 - settings.tolerance / 12, 2, 255));

    // 下塗り・外側到達判定では、壁として止めるかではなく「線が存在するか」を見る。
    // ColorTraceやMyPaintの薄いアンチエイリアス端も拾う。
    constexpr std::uint8_t kLinePresenceThreshold = 1U;

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
        addBitmapAlphaToMask(masks.underpaintBand, layerBitmap, width, height, kLinePresenceThreshold);
        addBitmapAlphaToMask(masks.exteriorBarrier, layerBitmap, width, height, kLinePresenceThreshold);
    }

    // 隙間閉じは「閉じていない線の小穴」をふさぐための壁膨張。
    // MyPaintの柔らかい線は端が薄くなるので、最低1pxだけ補強する。
    // ただし細いSimple線で色が外へ見える原因にもなるため、既定値は1pxに抑える。
    const int wallGrowRadius = std::max(1, std::clamp(settings.gapClosePx, 0, 8));
    dilateMask(masks.wall, width, height, wallGrowRadius);

    // 外側到達判定用の壁。ここは下塗りバンドほど太らせない。
    // 太らせすぎると外側判定が過剰になり、逆に線外の色残りを検出できなくなる。
    const int exteriorBarrierGrowRadius = std::max(1, std::min(wallGrowRadius, 2));
    dilateMask(masks.exteriorBarrier, width, height, exteriorBarrierGrowRadius);

    // 下塗りバンドは線そのものだけでなく、線のすぐ内側/下側の白フチも含める。
    // Step 18iでは、このバンド内でも「外側から到達できる領域」にはPaintを入れない。
    const int underpaintBandRadius = wallGrowRadius + underpaintIterationRadius(settings);
    masks.underpaintBand = dilatedMaskCopy(masks.underpaintBand, width, height, underpaintBandRadius);
    return masks;
}

int countMaskPixels(const std::vector<std::uint8_t>& mask)
{
    return static_cast<int>(std::count(mask.begin(), mask.end(), static_cast<std::uint8_t>(1U)));
}

std::vector<std::uint8_t> buildExteriorReachableMask(const std::vector<std::uint8_t>& exteriorBarrier,
                                                       int width,
                                                       int height)
{
    // キャンバス外周から、線を越えずに到達できる場所を外側領域として扱う。
    // 下塗りはこの外側領域へは広げない。
    // これにより、細いSimple線の外側へPaintが見える問題を防ぐ。
    std::vector<std::uint8_t> exterior(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
    if (width <= 0 || height <= 0 || exteriorBarrier.empty()) {
        return exterior;
    }

    std::vector<int> queue;
    queue.reserve(static_cast<std::size_t>(width + height) * 2U);

    auto enqueueIfOpen = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return;
        }
        const std::size_t idx = indexOf(x, y, width);
        if (exterior[idx] != 0U || exteriorBarrier[idx] != 0U) {
            return;
        }
        exterior[idx] = 1U;
        queue.push_back(static_cast<int>(idx));
    };

    for (int x = 0; x < width; ++x) {
        enqueueIfOpen(x, 0);
        enqueueIfOpen(x, height - 1);
    }
    for (int y = 0; y < height; ++y) {
        enqueueIfOpen(0, y);
        enqueueIfOpen(width - 1, y);
    }

    std::size_t head = 0U;
    while (head < queue.size()) {
        const int current = queue[head++];
        const int x = current % width;
        const int y = current / width;
        const int neighbors[4][2] = {{x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}};
        for (const auto& neighbor : neighbors) {
            enqueueIfOpen(neighbor[0], neighbor[1]);
        }
    }

    return exterior;
}

void bleedFilledMaskIntoUnderpaintBand(std::vector<std::uint8_t>& filled,
                                         const std::vector<std::uint8_t>& underpaintBand,
                                         const std::vector<std::uint8_t>& exteriorReachable,
                                         int width,
                                         int height,
                                         int radius)
{
    // 旧inset処理は塗りを境界から引っ込めるため、白い隙間の原因になった。
    // ただしStep 18hのように単純に線周辺へ広げるだけだと、細いSimple線では
    // Paintが線の外側へ見えることがある。
    // Step 18iでは、キャンバス外側から到達できる領域には下塗りを入れない。
    // これにより「MyPaint線では隙間を埋める」「細い線では外へ漏らさない」を両立する。
    const int safeRadius = std::max(0, radius);
    if (safeRadius <= 0 || filled.empty() || underpaintBand.empty() || exteriorReachable.empty()) {
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
                if (filled[center] != 0U || underpaintBand[center] == 0U || exteriorReachable[center] != 0U) {
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
                                           const std::vector<std::uint8_t>& exteriorReachable,
                                           int width,
                                           int height)
{
    // FloodFillのマスク上では埋まっていても、Paintは横スパンのブラシストロークとして保存される。
    // ブラシのアンチエイリアス・丸キャップ・整数化の都合で、ColorTrace線の直下に1pxだけ
    // 白点が残ることがあるため、線周辺バンド内の1px穴をPaint側の正データとして塞ぐ。
    if (filled.empty() || underpaintBand.empty() || exteriorReachable.empty()) {
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
                if (filled[center] != 0U || sealBand[center] == 0U || exteriorReachable[center] != 0U) {
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

    // Step 18hでは白点を消すためにPaintスパンを太くしすぎ、
    // 細いSimple線の外側へ塗り色が見える副作用があった。
    // Step 18iではマスク側で1px穴を塞ぎ、ストローク側は必要最小限の太さに戻す。
    stroke.radiusPx = 1.05f;

    const float fy = static_cast<float>(y) + 0.5f;
    if (x1 - x0 <= 1) {
        const float centerX = static_cast<float>(x0) + 0.5f;
        stroke.points.push_back(StrokePoint{centerX, fy, 1.0f});
    } else {
        const float startX = static_cast<float>(x0) + 0.25f;
        const float endX = static_cast<float>(x1) - 0.25f;
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
    const std::vector<std::uint8_t> exteriorReachable =
        buildExteriorReachableMask(masks.exteriorBarrier, width, height);

    const int underpaintRadius = underpaintIterationRadius(settings);
    bleedFilledMaskIntoUnderpaintBand(filled, masks.underpaintBand, exteriorReachable, width, height, underpaintRadius);
    sealOnePixelGapsForPaintRasterization(filled, masks.underpaintBand, exteriorReachable, width, height);

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
        result.message = "filled with exterior-guarded underpaint";
    } else {
        result.message = "no spans generated";
    }
    return result;
}

} // namespace perapera::fill
