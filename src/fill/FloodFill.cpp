// このファイルの役割:
// FloodFill.h の実装。
// Normal / ColorTrace レイヤーの線を壁としてラスタライズし、クリック位置から4方向に塗り広げる。
// Phase 1.5 Step 19:
//   - バケツ塗りは横スパン/境界ドットのブラシストローク群へ変換しない。
//   - FloodFillで得たピクセルマスクを FillStroke::bitmap として直接保存する。
//   - CanvasBitmapがFillStrokeを直接焼くため、ブラシの丸キャップ・アンチエイリアス変換誤差をなくす。
// Phase 1.5 Step 19g:
//   - FillStroke化後に不要になった下塗り滲み出し処理を削除する。
//   - BFSでwallを越えずに得たfilledマスクをそのまま0/255化して保存し、線外へのはみ出しを防ぐ。
// Phase 1.5 Step 19h:
//   - BFS確定後、filledに隣接するwallピクセルだけをfilledへ取り込み、
//     線のアンチエイリアス端と塗りの間に残る1px隙間を塞ぐ。
// Phase 1.5 Step 19i:
//   - 不透明な線芯までfilledへ取り込むと塗り色が線へまとわりついて見えるため、
//     lineCoreマスクを追加し、半透明エッジだけを取り込む。

#include "fill/FloodFill.h"

#include "brush/MyPaintBrushEngine.h"
#include "render/CanvasBitmap.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
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

struct WallMask {
    // BFSが越えない壁。線マスクを gapClosePx ぶん膨張したもの。
    std::vector<std::uint8_t> wall;

    // 線の不透明コア。ここは塗りに取り込まない。
    // 半透明エッジだけを取り込むことで、白フチを消しつつ線への色まとわりつきを避ける。
    std::vector<std::uint8_t> lineCore;
};

WallMask buildWallMask(const Frame& frame,
                       int targetLayerIndex,
                       int width,
                       int height,
                       const FloodFillSettings& settings)
{
    WallMask masks;
    masks.wall.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
    masks.lineCore.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);

    // tolerance が高いほど薄い線も壁として扱う。
    // MyPaintは線端や筆圧の弱い部分が低アルファになりやすいため、
    // ある程度低いしきい値で壁に入れる。
    const auto wallThreshold = static_cast<std::uint8_t>(
        std::clamp(24 - settings.tolerance / 12, 2, 255));

    // この値以上は線の芯として扱う。
    // wallには含めるが、filledへは取り込まない。
    constexpr std::uint8_t kLineCoreThreshold = 200U;

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
        addBitmapAlphaToMask(masks.lineCore, layerBitmap, width, height, kLineCoreThreshold);
    }

    // 線の小穴（閉じていない隙間）を塞ぐための膨張。
    // lineCoreは膨張しない。膨張したwallのうち、lineCoreでない場所だけが
    // 1px隙間塞ぎとしてfilledへ取り込まれる。
    const int wallGrowRadius = std::max(1, std::clamp(settings.gapClosePx, 0, 8));
    dilateMask(masks.wall, width, height, wallGrowRadius);

    return masks;
}

int countMaskPixels(const std::vector<std::uint8_t>& mask)
{
    return static_cast<int>(std::count(mask.begin(), mask.end(), static_cast<std::uint8_t>(1U)));
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

    const WallMask masks = buildWallMask(frame, targetLayerIndex, width, height, settings);
    const std::vector<std::uint8_t>& wall = masks.wall;
    const std::vector<std::uint8_t>& lineCore = masks.lineCore;
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

    // BFS確定後、wall上のピクセルのうち filled に隣接するものを filled に取り込む。
    // ただし lineCore（線の不透明コア）は取り込まない。
    // 半透明エッジだけを塗りの下地に含めることで、1px白フチを塞ぎつつ、
    // 線芯への色まとわりつきを防ぐ。
    // wallの外側にある非wall領域は追加しないため、Step 18系の滲み出し処理のような
    // 塗りのはみ出しは作らない。
    {
        constexpr int kDx[4] = {1, -1, 0, 0};
        constexpr int kDy[4] = {0, 0, 1, -1};
        bool changed = true;
        while (changed) {
            changed = false;
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const std::size_t idx = indexOf(x, y, width);
                    if (filled[idx] != 0U || wall[idx] == 0U || lineCore[idx] != 0U) {
                        continue;
                    }
                    for (int d = 0; d < 4; ++d) {
                        const int nx = x + kDx[d];
                        const int ny = y + kDy[d];
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                            continue;
                        }
                        if (filled[indexOf(nx, ny, width)] != 0U) {
                            filled[idx] = 1U;
                            changed = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    result.filledPixelCount = countMaskPixels(filled);
    if (result.filledPixelCount <= 0) {
        result.message = "nothing to fill";
        return result;
    }

    int minX = width;
    int minY = height;
    int maxX = -1;
    int maxY = -1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (filled[indexOf(x, y, width)] == 0U) {
                continue;
            }
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        }
    }
    if (maxX < minX || maxY < minY) {
        result.message = "nothing to fill";
        return result;
    }

    const int croppedWidth = maxX - minX + 1;
    const int croppedHeight = maxY - minY + 1;
    std::vector<std::uint8_t> croppedMask(static_cast<std::size_t>(croppedWidth) *
                                          static_cast<std::size_t>(croppedHeight), 0U);
    for (int y = 0; y < croppedHeight; ++y) {
        for (int x = 0; x < croppedWidth; ++x) {
            const std::uint8_t value = filled[indexOf(minX + x, minY + y, width)];
            croppedMask[static_cast<std::size_t>(y) * static_cast<std::size_t>(croppedWidth) +
                        static_cast<std::size_t>(x)] = value == 0U ? 0U : 255U;
        }
    }

    Stroke fillStroke;
    fillStroke.color = fillColor;
    fillStroke.opacity = 1.0f;
    fillStroke.brushEngine = StrokeBrushEngine::Fill;
    fillStroke.bitmap = std::move(croppedMask);
    fillStroke.bitmapX = minX;
    fillStroke.bitmapY = minY;
    fillStroke.bitmapWidth = croppedWidth;
    fillStroke.bitmapHeight = croppedHeight;
    result.strokes.push_back(std::move(fillStroke));
    result.success = true;
    result.message = "filled";
    return result;
}

} // namespace perapera::fill
