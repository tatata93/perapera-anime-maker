// このファイルの役割:
// FloodFill.h の実装。
// Normal / ColorTrace レイヤーの線を壁としてラスタライズし、クリック位置から4方向に塗り広げる。
// 現在のデータモデルはストローク正本なので、塗り結果は横方向の短いストローク列として返す。

#include "fill/FloodFill.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace perapera::fill {
namespace {

std::uint8_t toByte(float value)
{
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

std::size_t indexOf(int x, int y, int width)
{
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

bool isWallLayer(LayerType type)
{
    return type == LayerType::Normal || type == LayerType::ColorTrace;
}

void markWallPixel(std::vector<std::uint8_t>& wall,
                   int width,
                   int height,
                   int x,
                   int y,
                   std::uint8_t alpha,
                   std::uint8_t threshold)
{
    if (x < 0 || y < 0 || x >= width || y >= height || alpha < threshold) {
        return;
    }
    wall[indexOf(x, y, width)] = 1U;
}

void stampWallCircle(std::vector<std::uint8_t>& wall,
                     int width,
                     int height,
                     float cx,
                     float cy,
                     float radius,
                     std::uint8_t alpha,
                     std::uint8_t threshold)
{
    const int x0 = std::max(0, static_cast<int>(std::floor(cx - radius - 1.0f)));
    const int y0 = std::max(0, static_cast<int>(std::floor(cy - radius - 1.0f)));
    const int x1 = std::min(width, static_cast<int>(std::ceil(cx + radius + 1.0f)));
    const int y1 = std::min(height, static_cast<int>(std::ceil(cy + radius + 1.0f)));

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - cx;
            const float dy = static_cast<float>(y) + 0.5f - cy;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance > radius + 0.5f) {
                continue;
            }
            const float coverage = std::clamp(radius + 0.5f - distance, 0.0f, 1.0f);
            const auto coveredAlpha = static_cast<std::uint8_t>(std::lround(static_cast<float>(alpha) * coverage));
            markWallPixel(wall, width, height, x, y, coveredAlpha, threshold);
        }
    }
}

void rasterizeWallStroke(std::vector<std::uint8_t>& wall,
                         int width,
                         int height,
                         const Stroke& stroke,
                         float layerOpacity,
                         std::uint8_t threshold)
{
    if (stroke.points.empty()) {
        return;
    }

    const auto alpha = toByte(stroke.color[3] * layerOpacity);
    const float baseRadius = std::max(0.75f, stroke.radiusPx);
    const float spacing = std::max(0.5f, baseRadius * 0.5f);

    if (stroke.points.size() == 1U) {
        const StrokePoint& point = stroke.points.front();
        stampWallCircle(wall, width, height, point.x, point.y, baseRadius, alpha, threshold);
        return;
    }

    for (std::size_t pointIndex = 1; pointIndex < stroke.points.size(); ++pointIndex) {
        const StrokePoint& from = stroke.points[pointIndex - 1U];
        const StrokePoint& to = stroke.points[pointIndex];
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        const int steps = std::max(1, static_cast<int>(std::ceil(length / spacing)));
        for (int step = 0; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            const float x = from.x + dx * t;
            const float y = from.y + dy * t;
            const float pressure = from.pressure + (to.pressure - from.pressure) * t;
            stampWallCircle(wall, width, height, x, y, baseRadius * std::max(0.1f, pressure), alpha, threshold);
        }
    }
}

void dilateWallMask(std::vector<std::uint8_t>& wall, int width, int height, int radius)
{
    if (radius <= 0 || wall.empty()) {
        return;
    }

    const std::vector<std::uint8_t> original = wall;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (original[indexOf(x, y, width)] == 0U) {
                continue;
            }
            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    if (dx * dx + dy * dy > radius * radius) {
                        continue;
                    }
                    const int nx = x + dx;
                    const int ny = y + dy;
                    if (nx >= 0 && ny >= 0 && nx < width && ny < height) {
                        wall[indexOf(nx, ny, width)] = 1U;
                    }
                }
            }
        }
    }
}

int countMaskPixels(const std::vector<std::uint8_t>& mask)
{
    return static_cast<int>(std::count(mask.begin(), mask.end(), static_cast<std::uint8_t>(1U)));
}

void insetFilledMask(std::vector<std::uint8_t>& filled,
                     const std::vector<std::uint8_t>& wall,
                     int width,
                     int height,
                     int radius)
{
    if (radius <= 0 || filled.empty()) {
        return;
    }

    const std::vector<std::uint8_t> original = filled;
    std::fill(filled.begin(), filled.end(), 0U);

    // 塗り領域の境界から radius px ぶん内側だけを残す。
    // これにより、線画に密着しすぎたPaintストロークを少し引っ込めて、
    // 主線の上へ色が食い込む見た目を避ける。
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t centerIndex = indexOf(x, y, width);
            if (original[centerIndex] == 0U) {
                continue;
            }

            bool keep = true;
            for (int dy = -radius; dy <= radius && keep; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    if (dx * dx + dy * dy > radius * radius) {
                        continue;
                    }
                    const int nx = x + dx;
                    const int ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                        keep = false;
                        break;
                    }
                    const std::size_t neighborIndex = indexOf(nx, ny, width);
                    if (original[neighborIndex] == 0U || wall[neighborIndex] != 0U) {
                        keep = false;
                        break;
                    }
                }
            }

            if (keep) {
                filled[centerIndex] = 1U;
            }
        }
    }
}

std::vector<std::uint8_t> buildWallMask(const Frame& frame,
                                        int targetLayerIndex,
                                        int width,
                                        int height,
                                        const FloodFillSettings& settings)
{
    std::vector<std::uint8_t> wall(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0U);
    // tolerance が高いほど、薄いアンチエイリアス部分も壁として扱う。
    // しきい値を高くしすぎると線の端で塗り漏れが起きるため、初期値はやや低めにする。
    const auto threshold = static_cast<std::uint8_t>(
        std::clamp(96 - settings.tolerance / 3, 1, 255));

    for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
        if (layerIndex == targetLayerIndex) {
            continue;
        }
        const Layer& layer = frame.layers[static_cast<std::size_t>(layerIndex)];
        if (!layer.visible || layer.opacity <= 0.0f || !isWallLayer(layer.type)) {
            continue;
        }
        for (const Stroke& stroke : layer.strokes) {
            rasterizeWallStroke(wall, width, height, stroke, layer.opacity, threshold);
        }
    }

    dilateWallMask(wall, width, height, std::clamp(settings.gapClosePx, 0, 6));
    return wall;
}

Stroke makeSpanStroke(int y, int x0, int x1, const std::array<float, 4>& color)
{
    Stroke stroke;
    stroke.color = color;
    stroke.radiusPx = 0.65f;

    const float fy = static_cast<float>(y) + 0.5f;
    const float startX = static_cast<float>(x0) + 0.5f;
    const float endX = static_cast<float>(x1) - 0.5f;
    if (x1 - x0 <= 1) {
        stroke.points.push_back(StrokePoint{startX, fy, 1.0f});
    } else {
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

    const std::vector<std::uint8_t> wall = buildWallMask(frame, targetLayerIndex, width, height, settings);
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

    // シード点の位置は「漏れ」の判定に使わない。
    // 実際の漏れ判定は、塗り面積が上限を超えたかどうかだけで行う。
    std::size_t head = 0U;
    while (head < queue.size()) {
        const int current = queue[head++];
        const int x = current % width;
        const int y = current / width;

        // 漏れ防止は、最後まで塗ってから判定するだけだと巨大な塗りストロークを
        // 作る直前まで進んでしまう。探索中にも面積上限を超えたらすぐ止める。
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

    if (leakGuardPercent > 0) {
        // 面積が上限を超えた場合だけ漏れとして中止する。
        // キャンバス端への到達は正当なケース（背景全体塗りなど）があるため、
        // 単独の漏れ判定には使わない。
        if (maxAllowedPixels > 0 && rawFilledPixelCount >= maxAllowedPixels) {
            result.message = "fill leaked: area exceeded guard percent ("
                + std::to_string(leakGuardPercent) + "%)";
            return result;
        }
    }

    insetFilledMask(filled, wall, width, height, std::clamp(settings.insetPx, 0, 8));
    result.filledPixelCount = countMaskPixels(filled);
    if (result.filledPixelCount <= 0) {
        result.message = "fill area too thin after inset; lower inset px";
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
        result.message = "filled";
    } else {
        result.message = "no spans generated";
    }
    return result;
}

} // namespace perapera::fill
