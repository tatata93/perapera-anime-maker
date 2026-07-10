#include "core/CameraResolver.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace perapera;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

bool nearlyEqual(float a, float b)
{
    return std::fabs(a - b) < 0.001f;
}

} // namespace

int main()
{
    CameraSettings camera;
    camera.centerX = 100.0f;
    camera.centerY = 200.0f;
    camera.zoom = 0.0f;
    camera.keys.push_back(CameraKey{4, 400.0f, 800.0f, 4.0f});

    ResolvedCamera2D base = resolveCameraAtFrame(camera, 4);
    require(nearlyEqual(base.centerX, 100.0f), "disabled animation uses base centerX");
    require(nearlyEqual(base.centerY, 200.0f), "disabled animation uses base centerY");
    require(nearlyEqual(base.zoom, 0.001f), "base zoom is clamped");

    camera.animationEnabled = true;
    camera.keys.clear();
    camera.keys.push_back(CameraKey{8, 800.0f, 1600.0f, 8.0f});
    camera.keys.push_back(CameraKey{2, 200.0f, 400.0f, 2.0f});
    camera.keys.push_back(CameraKey{4, 400.0f, 800.0f, 4.0f});

    ResolvedCamera2D before = resolveCameraAtFrame(camera, 1);
    require(nearlyEqual(before.centerX, 200.0f), "before first key holds first centerX");
    require(nearlyEqual(before.zoom, 2.0f), "before first key holds first zoom");

    ResolvedCamera2D exact = resolveCameraAtFrame(camera, 4);
    require(nearlyEqual(exact.centerX, 400.0f), "exact key centerX");
    require(nearlyEqual(exact.centerY, 800.0f), "exact key centerY");

    ResolvedCamera2D between = resolveCameraAtFrame(camera, 6);
    require(nearlyEqual(between.centerX, 600.0f), "between keys linear centerX");
    require(nearlyEqual(between.centerY, 1200.0f), "between keys linear centerY");
    require(nearlyEqual(between.zoom, 6.0f), "between keys linear zoom");

    ResolvedCamera2D after = resolveCameraAtFrame(camera, 12);
    require(nearlyEqual(after.centerX, 800.0f), "after last key holds last centerX");
    require(nearlyEqual(after.zoom, 8.0f), "after last key holds last zoom");

    camera.keys.push_back(CameraKey{6, 601.0f, 1201.0f, -1.0f});
    camera.keys.push_back(CameraKey{6, 602.0f, 1202.0f, 0.5f});
    ResolvedCamera2D duplicate = resolveCameraAtFrame(camera, 6);
    require(nearlyEqual(duplicate.centerX, 602.0f), "duplicate exact key uses last matching key");
    require(nearlyEqual(duplicate.zoom, 0.5f), "duplicate exact key zoom");

    std::cout << "perapera_camera_resolver_selftest passed\n";
    return 0;
}
