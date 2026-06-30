// Scene Plate canvas preview split from AppDrawingMode.
#include "ui/App.h"

#include <filesystem>

#include <imgui.h>

#include "ui/AppProjectIOSupport.h"
#include "ui/AppScenePlateSupport.h"

namespace perapera {

void App::drawScenePlateCanvasPreview(int timelineFrame, ImVec2 areaMin, ImVec2 areaSize, ImDrawList* drawList)
{
    if (!scenePlateCanvasPreviewEnabled_) {
        return;
    }
    const std::filesystem::path scenePlateProjectFolder = appio::absolutePath(exportState_.projectFolder);
    scene_plate_ui::drawScenePlatePreviewStackOnCanvas(
        workingScenePlates_,
        timelineFrame,
        project_.canvas.width,
        project_.canvas.height,
        scenePlateProjectFolder,
        renderer_,
        scenePlateImageCache_,
        canvasView_,
        areaMin,
        areaSize,
        drawList);
}

} // namespace perapera
