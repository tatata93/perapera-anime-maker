#pragma once

#include <vector>

namespace perapera {

struct CameraKey {
    int frame = 0;
    float centerX = 1280.0f;
    float centerY = 720.0f;
    float zoom = 1.0f;
};

struct CameraSettings {
    float centerX = 1280.0f;
    float centerY = 720.0f;
    float zoom = 1.0f;
    bool animationEnabled = false;
    std::vector<CameraKey> keys;
};

} // namespace perapera
