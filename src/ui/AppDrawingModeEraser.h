#pragma once

#include <vector>

#include "core/Stroke.h"

namespace perapera::app_drawing {

std::vector<Stroke> splitStrokeByEraser(const Stroke& stroke,
                                        const Stroke& eraserStroke,
                                        float eraserRadius,
                                        bool& changed);

} // namespace perapera::app_drawing
