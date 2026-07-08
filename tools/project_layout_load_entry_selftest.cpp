#include <chrono>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <string>
#include <vector>

#include "core/Cell.h"
#include "core/Cut.h"
#include "core/Layer.h"
#include "core/Scene.h"
#include "core/StrokePoint.h"
#include "io/ProjectLayoutLoadEntry.h"
#include "io/ProjectLayoutSaveEntry.h"

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "selftest failed: " << message << '\n';
    }
    return condition;
}

bool nearlyEqual(float left, float right) {
    return std::fabs(left - right) < 0.0001f;
}

const perapera::Cell* findCellById(const std::vector<perapera::Cell>& cells, const std::string& id) {
    for (const perapera::Cell& cell : cells) {
        if (cell.id == id) {
            return &cell;
        }
    }
    return nullptr;
}

void removeAllNoThrow(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::remove_all(path, error);
    if (error) {
        std::cerr << "cleanup warning: " << error.message() << '\n';
    }
}

std::filesystem::path uniqueSelftestRoot() {
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / ("perapera_project_layout_load_entry_selftest_" + std::to_string(ticks));
}

perapera::Cell makeCell(std::string id, std::string category, int frameCount, int layerCount) {
    perapera::Cell cell;
    cell.id = std::move(id);
    cell.name = cell.id;
    cell.category = std::move(category);
    cell.opacity = 0.75f;
    cell.placement.x = 12.0f;
    cell.placement.y = 34.0f;

    for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        perapera::Frame frame;
        frame.name = "F" + std::to_string(frameIndex + 1);
        frame.durationFrames = frameIndex + 1;

        for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            perapera::Layer layer;
            layer.layerId = "layer_" + std::to_string(frameIndex + 1) + "_" + std::to_string(layerIndex + 1);
            layer.name = "Layer " + std::to_string(layerIndex + 1);
            layer.opacity = 0.5f;
            layer.type = layerIndex == 0 ? perapera::LayerType::Paint : perapera::LayerType::Rough;

            perapera::Stroke stroke;
            stroke.brushEngine = perapera::StrokeBrushEngine::MyPaint;
            stroke.color = {0.1f, 0.2f, 0.3f, 0.4f};
            stroke.radiusPx = 9.0f;
            stroke.opacity = 0.66f;
            stroke.hardness = 0.44f;
            stroke.spacing = 0.33f;
            stroke.pressureToSize = 0.22f;
            stroke.pressureToOpacity = 0.11f;
            stroke.watercolorBleed = 0.12f;
            stroke.colorMix = 0.13f;
            stroke.dryRate = 0.14f;
            perapera::StrokePoint point;
            point.x = 100.0f + static_cast<float>(frameIndex);
            point.y = 200.0f + static_cast<float>(layerIndex);
            point.pressure = 0.8f;
            stroke.points.push_back(point);
            layer.strokes.push_back(stroke);

            if (cell.id == "cell_A" && frameIndex == 0 && layerIndex == 0) {
                perapera::Stroke fillStroke;
                fillStroke.brushEngine = perapera::StrokeBrushEngine::Fill;
                fillStroke.color = {0.9f, 0.8f, 0.7f, 1.0f};
                fillStroke.bitmapX = 3;
                fillStroke.bitmapY = 4;
                fillStroke.bitmapWidth = 2;
                fillStroke.bitmapHeight = 2;
                fillStroke.bitmap = {0, 255, 128, 64};
                layer.strokes.push_back(fillStroke);
            }

            frame.layers.push_back(layer);
        }
        cell.frames.push_back(frame);
    }

    return cell;
}

} // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path root = uniqueSelftestRoot();
    removeAllNoThrow(root);

    perapera::Scene scene;
    scene.id = "scene_001";
    scene.name = "Scene 1";

    perapera::Cut cut;
    cut.id = "cut_001";
    cut.name = "C001";
    cut.totalFrames = 24;
    cut.frameRate = 30;
    cut.timesheet.totalFrames = 24;
    cut.cellZOrderKeys = {"cell_BG", "cell_A"};

    std::vector<perapera::Cell> cells;
    cells.push_back(makeCell("cell_A", "character", 2, 2));
    cells.push_back(makeCell("cell_BG", "background", 1, 1));

    std::string error;
    if (!require(perapera::saveProjectNewLayoutMinimal(root, scene, cut, cells, nullptr, &error), "saveProjectNewLayoutMinimal failed")) {
        std::cerr << "error: " << error << '\n';
        removeAllNoThrow(root);
        return 1;
    }

    perapera::ProjectLayoutLoadResult loaded;
    if (!require(perapera::loadProjectNewLayoutMinimal(root, {}, &loaded, &error), "loadProjectNewLayoutMinimal failed")) {
        std::cerr << "error: " << error << '\n';
        removeAllNoThrow(root);
        return 1;
    }

    if (!require(loaded.scene.id == "scene_001", "wrong scene id") ||
        !require(loaded.cut.id == "cut_001", "wrong cut id") ||
        !require(loaded.cut.totalFrames == 24, "wrong cut totalFrames") ||
        !require(loaded.project.timeline.totalFrames == 24, "wrong project totalFrames") ||
        !require(loaded.project.output.fps == 30, "wrong project fps") ||
        !require(loaded.project.cells.size() == 2, "wrong project cell count") ||
        !require(loaded.project.cellOrder.size() == 2, "wrong project cellOrder count") ||
        !require(loaded.project.cellOrder[0] == "cell_BG", "wrong first cellOrder") ||
        !require(loaded.project.cellOrder[1] == "cell_A", "wrong second cellOrder") ||
        !require(loaded.project.cells[0].id == "cell_BG", "wrong first project cell") ||
        !require(loaded.cellCount == 2, "wrong result cellCount") ||
        !require(loaded.frameCount == 3, "wrong result frameCount") ||
        !require(loaded.layerCount == 5, "wrong result layerCount") ||
        !require(loaded.strokeCount == 6, "wrong result strokeCount")) {
        removeAllNoThrow(root);
        return 1;
    }

    const perapera::Cell* cellA = findCellById(loaded.project.cells, "cell_A");
    if (!require(cellA != nullptr, "cell_A was not loaded")) {
        removeAllNoThrow(root);
        return 1;
    }
    if (!require(cellA->frames.size() == 2, "wrong frame count for cell_A") ||
        !require(cellA->frames[0].layers.size() == 2, "wrong layer count for cell_A/F1")) {
        removeAllNoThrow(root);
        return 1;
    }

    const perapera::Layer& loadedLayer = cellA->frames[0].layers[0];
    if (!require(loadedLayer.type == perapera::LayerType::Paint, "wrong loaded layer type") ||
        !require(loadedLayer.strokes.size() == 2, "wrong stroke count for cell_A/F1/L1")) {
        removeAllNoThrow(root);
        return 1;
    }

    const perapera::Stroke& loadedStroke = loadedLayer.strokes[0];
    const perapera::Stroke& loadedFill = loadedLayer.strokes[1];
    if (!require(loadedStroke.brushEngine == perapera::StrokeBrushEngine::MyPaint, "wrong brush engine") ||
        !require(nearlyEqual(loadedStroke.color[2], 0.3f), "wrong stroke color") ||
        !require(nearlyEqual(loadedStroke.opacity, 0.66f), "wrong stroke opacity") ||
        !require(nearlyEqual(loadedStroke.hardness, 0.44f), "wrong stroke hardness") ||
        !require(nearlyEqual(loadedStroke.spacing, 0.33f), "wrong stroke spacing") ||
        !require(nearlyEqual(loadedStroke.pressureToSize, 0.22f), "wrong stroke pressureToSize") ||
        !require(nearlyEqual(loadedStroke.pressureToOpacity, 0.11f), "wrong stroke pressureToOpacity") ||
        !require(nearlyEqual(loadedStroke.watercolorBleed, 0.12f), "wrong stroke watercolorBleed") ||
        !require(nearlyEqual(loadedStroke.colorMix, 0.13f), "wrong stroke colorMix") ||
        !require(nearlyEqual(loadedStroke.dryRate, 0.14f), "wrong stroke dryRate") ||
        !require(loadedStroke.points.size() == 1, "wrong point count") ||
        !require(loadedFill.brushEngine == perapera::StrokeBrushEngine::Fill, "wrong fill engine") ||
        !require(loadedFill.bitmapX == 3 && loadedFill.bitmapY == 4, "wrong fill bitmap origin") ||
        !require(loadedFill.bitmapWidth == 2 && loadedFill.bitmapHeight == 2, "wrong fill bitmap size") ||
        !require(loadedFill.bitmap.size() == 4 && loadedFill.bitmap[1] == 255 && loadedFill.bitmap[2] == 128,
                 "wrong fill bitmap data")) {
        removeAllNoThrow(root);
        return 1;
    }

    removeAllNoThrow(root);
    std::cout << "perapera_project_layout_load_entry_selftest passed\n";
    return 0;
}
