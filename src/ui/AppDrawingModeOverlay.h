#pragma once

#include <imgui.h>

#include "core/Frame.h"
#include "core/Stroke.h"
#include "render/CanvasRenderer.h"

namespace perapera::app_drawing {

void drawOnionFrameDirect(const Frame& frame,
                          bool isPrevious,
                          float opacity,
                          const CanvasView& view,
                          ImVec2 areaMin,
                          ImVec2 areaSize,
                          ImDrawList* drawList);

void drawLightweightEraserPreview(const Stroke& eraserStroke,
                                  const CanvasView& view,
                                  ImVec2 areaMin,
                                  ImVec2 areaSize,
                                  ImDrawList* drawList);

} // namespace perapera::app_drawing
