#include "project/DrawingNewLayoutIO.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace {

fs::path makeTempRoot()
{
    return fs::temp_directory_path() / "perapera_drawing_new_layout_io_selftest";
}

perapera::DrawingCell makeCell()
{
    perapera::DrawingCell cell;
    cell.id = "cell_BG";
    cell.name = "Background";
    cell.category = perapera::DrawingCellCategory::Background;
    cell.widthPx = 3840;
    cell.heightPx = 2160;
    cell.positionX = 12.0f;
    cell.positionY = -8.0f;
    cell.scale = 1.25f;
    cell.rotationDegrees = 3.0f;

    perapera::AnimationFrame frame;
    frame.name = "F001";
    frame.durationFrames = 2;

    perapera::DrawingLayer layer;
    layer.name = "Line";
    layer.opacity = 0.75f;

    perapera::Stroke stroke;
    stroke.color.r = 0.1f;
    stroke.color.g = 0.2f;
    stroke.color.b = 0.3f;
    stroke.color.a = 1.0f;
    stroke.radiusPx = 6.0f;
    stroke.points.push_back({10.0f, 20.0f});
    stroke.points.push_back({30.0f, 40.0f});
    stroke.invalidateBounds();

    layer.strokes.push_back(stroke);
    frame.layers.push_back(layer);
    cell.frames.push_back(frame);
    return cell;
}

bool check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

} // namespace

int main()
{
    const fs::path root = makeTempRoot();
    std::error_code cleanupError;
    fs::remove_all(root, cleanupError);

    perapera::DrawingNewLayoutIOOptions options;
    options.sceneId = "scene_001";
    options.cutId = "cut_001";

    std::vector<perapera::DrawingCell> cells;
    cells.push_back(makeCell());

    std::string errorMessage;
    perapera::DrawingNewLayoutSaveResult saveResult;
    if (!perapera::saveDrawingCellsToNewLayout(root, options, cells, &saveResult, &errorMessage)) {
        std::cerr << "save failed: " << errorMessage << std::endl;
        return 1;
    }

    if (!check(saveResult.cellCount == 1, "cell count mismatch after save")) {
        return 1;
    }
    if (!check(saveResult.frameCount == 1, "frame count mismatch after save")) {
        return 1;
    }
    if (!check(saveResult.layerCount == 1, "layer count mismatch after save")) {
        return 1;
    }
    if (!check(saveResult.strokeCount == 1, "stroke count mismatch after save")) {
        return 1;
    }

    perapera::DrawingNewLayoutLoadResult loadResult;
    if (!perapera::loadDrawingCellsFromNewLayout(root, options, &loadResult, &errorMessage)) {
        std::cerr << "load failed: " << errorMessage << std::endl;
        return 1;
    }

    if (!check(loadResult.cells.size() == 1, "cell count mismatch after load")) {
        return 1;
    }
    const perapera::DrawingCell& loadedCell = loadResult.cells.front();
    if (!check(loadedCell.id == "cell_BG", "loaded cell id mismatch")) {
        return 1;
    }
    if (!check(loadedCell.category == perapera::DrawingCellCategory::Background, "loaded category mismatch")) {
        return 1;
    }
    if (!check(loadedCell.frames.size() == 1, "loaded frame count mismatch")) {
        return 1;
    }
    if (!check(loadedCell.frames.front().layers.size() == 1, "loaded layer count mismatch")) {
        return 1;
    }
    if (!check(loadedCell.frames.front().layers.front().strokes.size() == 1, "loaded stroke count mismatch")) {
        return 1;
    }
    if (!check(loadedCell.frames.front().layers.front().strokes.front().points.size() == 2, "loaded point count mismatch")) {
        return 1;
    }

    fs::remove_all(root, cleanupError);
    std::cout << "perapera_drawing_new_layout_io_selftest passed" << std::endl;
    return 0;
}
