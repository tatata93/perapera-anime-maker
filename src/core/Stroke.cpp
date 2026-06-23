// このファイルの役割:
// Stroke の小さな補助処理を実装する。
// UIやSDLには依存しない。

#include "core/Stroke.h"

#include <algorithm>

namespace perapera {

bool Stroke::empty() const
{
    return points.empty();
}

void Stroke::addPoint(const StrokePoint& point)
{
    points.push_back(point);
}

StrokeBounds Stroke::bounds() const
{
    StrokeBounds result;
    if (points.empty()) {
        return result;
    }

    float minX = points.front().x;
    float minY = points.front().y;
    float maxX = points.front().x;
    float maxY = points.front().y;

    for (const StrokePoint& point : points) {
        minX = std::min(minX, point.x);
        minY = std::min(minY, point.y);
        maxX = std::max(maxX, point.x);
        maxY = std::max(maxY, point.y);
    }

    result.minX = minX - radiusPx;
    result.minY = minY - radiusPx;
    result.maxX = maxX + radiusPx;
    result.maxY = maxY + radiusPx;
    result.valid = true;
    return result;
}

} // namespace perapera
