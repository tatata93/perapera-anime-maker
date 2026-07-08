#pragma once

#include "core/Frame.h"
#include "core/Stroke.h"

namespace perapera::app_drawing {

Frame previewFrameWithEraser(const Frame& frame, int activeLayerIndex, const Stroke& eraserStroke);

} // namespace perapera::app_drawing
