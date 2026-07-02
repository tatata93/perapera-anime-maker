// このファイルの役割:
// Strokeの小さなユーティリティと外接矩形計算を実装する。

#include "core/Stroke.h"

#include <algorithm>
#include <string>

namespace perapera {

const char* strokeBrushEngineToString(StrokeBrushEngine engine)
{
    switch (engine) {
    case StrokeBrushEngine::Simple:
        return "Simple";
    case StrokeBrushEngine::MyPaint:
        return "MyPaint";
    case StrokeBrushEngine::Fill:
        return "Fill";
    }
    return "Simple";
}

StrokeBrushEngine strokeBrushEngineFromString(const std::string& value)
{
    if (value == "MyPaint" || value == "MyPaintBrushEngine") {
        return StrokeBrushEngine::MyPaint;
    }
    if (value == "Fill" || value == "FillStroke") {
        return StrokeBrushEngine::Fill;
    }
    return StrokeBrushEngine::Simple;
}

bool Stroke::empty() const
{
    if (brushEngine == StrokeBrushEngine::Fill) {
        return bitmap.empty() || bitmapWidth <= 0 || bitmapHeight <= 0;
    }
    return points.empty();
}

void Stroke::addPoint(const StrokePoint& point)
{
    points.push_back(point);
}

StrokeBounds Stroke::bounds() const
{
    StrokeBounds result;
    if (brushEngine == StrokeBrushEngine::Fill) {
        if (bitmap.empty() || bitmapWidth <= 0 || bitmapHeight <= 0) {
            return result;
        }
        result.minX = static_cast<float>(bitmapX);
        result.minY = static_cast<float>(bitmapY);
        result.maxX = static_cast<float>(bitmapX + bitmapWidth);
        result.maxY = static_cast<float>(bitmapY + bitmapHeight);
        result.valid = true;
        return result;
    }

    if (points.empty()) {
        return result;
    }

    result.minX = points.front().x;
    result.minY = points.front().y;
    result.maxX = points.front().x;
    result.maxY = points.front().y;
    result.valid = true;

    for (const StrokePoint& point : points) {
        result.minX = std::min(result.minX, point.x);
        result.minY = std::min(result.minY, point.y);
        result.maxX = std::max(result.maxX, point.x);
        result.maxY = std::max(result.maxY, point.y);
    }

    const float padding = std::max(1.0f, radiusPx + 2.0f);
    result.minX -= padding;
    result.minY -= padding;
    result.maxX += padding;
    result.maxY += padding;
    return result;
}

} // namespace perapera
