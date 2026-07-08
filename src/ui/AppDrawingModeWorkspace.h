#pragma once

#include <functional>

namespace perapera::app_drawing {

struct DrawingWorkspaceLayoutCallbacks {
    std::function<void()> drawLeftSidebar;
    std::function<void(float)> drawCanvasArea;
    std::function<void()> drawRightSidebar;
    std::function<void()> drawTimelineArea;
};

void drawDrawingWorkspaceLayout(const DrawingWorkspaceLayoutCallbacks& callbacks);

} // namespace perapera::app_drawing