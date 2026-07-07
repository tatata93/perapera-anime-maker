#include <chrono>
#include <filesystem>
#include <iostream>
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

            perapera::Stroke stroke;
            stroke.radiusPx = 9.0f;
            perapera::StrokePoint point;
            point.x = 100.0f + static_cast<float>(frameIndex);
            point.y = 200.0f + static_cast<float>(layerIndex);
            point.pressure = 0.8f;
            stroke.points.push_back(point);
            layer.strokes.push_back(stroke);

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
    cut.timesheet.totalFrames = 24;
    cut.cellZOrderKeys = {"cell_A", "cell_BG"};

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
        !require(loaded.project.cells.size() == 2, "wrong project cell count") ||
        !require(loaded.project.cellOrder.size() == 2, "wrong project cellOrder count") ||
        !require(loaded.project.cellOrder[0] == "cell_A", "wrong first cellOrder") ||
        !require(loaded.project.cellOrder[1] == "cell_BG", "wrong second cellOrder") ||
        !require(loaded.project.cells[0].frames.size() == 2, "wrong frame count for cell_A") ||
        !require(loaded.project.cells[0].frames[0].layers.size() == 2, "wrong layer count for cell_A/F1") ||
        !require(loaded.project.cells[0].frames[0].layers[0].strokes.size() == 1, "wrong stroke count") ||
        !require(loaded.project.cells[0].frames[0].layers[0].strokes[0].points.size() == 1, "wrong point count") ||
        !require(loaded.cellCount == 2, "wrong result cellCount") ||
        !require(loaded.frameCount == 3, "wrong result frameCount") ||
        !require(loaded.layerCount == 5, "wrong result layerCount") ||
        !require(loaded.strokeCount == 5, "wrong result strokeCount")) {
        removeAllNoThrow(root);
        return 1;
    }

    removeAllNoThrow(root);
    std::cout << "perapera_project_layout_load_entry_selftest passed\n";
    return 0;
}
