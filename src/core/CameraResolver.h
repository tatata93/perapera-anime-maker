#pragma once

#include "core/CameraSettings.h"

namespace perapera {

struct ResolvedCamera2D {
    float centerX = 1280.0f;
    float centerY = 720.0f;
    float zoom = 1.0f;
};

ResolvedCamera2D resolveCameraAtFrame(const CameraSettings& camera, int timelineFrame);

} // namespace perapera
