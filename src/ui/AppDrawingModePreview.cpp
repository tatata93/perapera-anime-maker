#include "ui/AppDrawingModePreview.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "ui/AppDrawingModeEraser.h"

namespace perapera::app_drawing {

Frame previewFrameWithEraser(const Frame& frame, int activeLayerIndex, const Stroke& eraserStroke)
{
    Frame preview = frame;
    if (eraserStroke.points.empty() || activeLayerIndex < 0 || activeLayerIndex >= static_cast<int>(preview.layers.size())) {
        return preview;
    }

    Layer& layer = preview.layers[static_cast<std::size_t>(activeLayerIndex)];
    std::vector<Stroke> rewrittenStrokes;
    rewrittenStrokes.reserve(layer.strokes.size());

    bool changed = false;
    const float radius = std::max(1.0f, eraserStroke.radiusPx);
    for (const Stroke& stroke : layer.strokes) {
        std::vector<Stroke> parts = splitStrokeByEraser(stroke, eraserStroke, radius, changed);
        rewrittenStrokes.insert(
            rewrittenStrokes.end(),
            std::make_move_iterator(parts.begin()),
            std::make_move_iterator(parts.end()));
    }

    if (changed) {
        layer.strokes = std::move(rewrittenStrokes);
    }
    return preview;
}

} // namespace perapera::app_drawing
