#include <iostream>

#include "core/Cell.h"
#include "core/Layer.h"
#include "core/Project.h"
#include "core/StrokePoint.h"
#include "ui/AppProjectIOSupport.h"

namespace {

bool require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "selftest failed: " << message << '\n';
    }
    return condition;
}

perapera::Project makeProject()
{
    perapera::Project project;
    project.name = "Signature Project";
    project.canvas.width = 3200;
    project.canvas.height = 1800;
    project.output.width = 2048;
    project.output.height = 1152;
    project.output.fps = 30;
    project.output.pixelAspect = 1.25f;
    project.timeline.totalFrames = 24;
    project.audio.enabled = true;
    project.audio.filePath = "audio/dialog.wav";
    project.audio.startFrame = 3;
    project.camera.centerX = 1600.0f;
    project.camera.centerY = 900.0f;
    project.camera.zoom = 1.2f;
    project.camera.animationEnabled = true;
    project.camera.keys.push_back({4, 1500.0f, 800.0f, 1.4f});

    perapera::Cell cell;
    cell.id = "cell_A";
    cell.name = "Cell A";
    cell.category = "character";
    cell.opacity = 0.75f;

    perapera::CellMotionKey motionKey;
    motionKey.frame = 7;
    motionKey.placement.x = 44.0f;
    motionKey.placement.scale = 1.25f;
    motionKey.interpolation = "ease";
    cell.motionKeys.push_back(motionKey);

    perapera::Frame frame;
    frame.name = "F001";
    frame.durationFrames = 2;

    perapera::Layer layer;
    layer.layerId = "layer_001";
    layer.name = "Paint";
    layer.type = perapera::LayerType::Paint;

    perapera::Stroke stroke;
    stroke.brushEngine = perapera::StrokeBrushEngine::MyPaint;
    stroke.color = {0.1f, 0.2f, 0.3f, 0.4f};
    stroke.radiusPx = 9.0f;
    stroke.opacity = 0.66f;
    stroke.hardness = 0.44f;
    stroke.points.push_back({100.0f, 200.0f, 0.8f});
    layer.strokes.push_back(stroke);

    perapera::Stroke fillStroke;
    fillStroke.brushEngine = perapera::StrokeBrushEngine::Fill;
    fillStroke.bitmapX = 3;
    fillStroke.bitmapY = 4;
    fillStroke.bitmapWidth = 2;
    fillStroke.bitmapHeight = 2;
    fillStroke.bitmap = {0, 255, 128, 64};
    layer.strokes.push_back(fillStroke);

    frame.layers.push_back(layer);
    cell.frames.push_back(frame);
    project.cells.push_back(cell);
    project.cellOrder.push_back(cell.id);
    return project;
}

} // namespace

int main()
{
    perapera::Project project = makeProject();
    const std::uint64_t baseSignature = perapera::appio::projectSignature(project);
    const perapera::appio::ProjectStats baseStats = perapera::appio::collectProjectStats(project);

    perapera::Project metadataChanged = project;
    metadataChanged.output.width += 1;
    if (!require(perapera::appio::projectSignature(metadataChanged) != baseSignature,
                 "metadata change did not affect signature") ||
        !require(perapera::appio::sameStats(baseStats, perapera::appio::collectProjectStats(metadataChanged)),
                 "metadata change should not affect stats")) {
        return 1;
    }

    perapera::Project motionChanged = project;
    motionChanged.cells[0].motionKeys[0].interpolation = "hold";
    if (!require(perapera::appio::projectSignature(motionChanged) != baseSignature,
                 "motion key change did not affect signature")) {
        return 1;
    }

    perapera::Project fillChanged = project;
    fillChanged.cells[0].frames[0].layers[0].strokes[1].bitmap[2] = 1;
    if (!require(perapera::appio::projectSignature(fillChanged) != baseSignature,
                 "fill bitmap sample change did not affect signature")) {
        return 1;
    }

    perapera::Project pointChanged = project;
    pointChanged.cells[0].frames[0].layers[0].strokes[0].points[0].pressure = 0.1f;
    if (!require(perapera::appio::projectSignature(pointChanged) != baseSignature,
                 "point pressure change did not affect signature")) {
        return 1;
    }

    if (!require(perapera::appio::validSelection(project, {0, 0, 0, true}), "valid selection rejected") ||
        !require(!perapera::appio::validSelection(project, {1, 0, 0, true}), "invalid selection accepted")) {
        return 1;
    }

    std::cout << "perapera_app_project_io_support_selftest passed\n";
    return 0;
}
