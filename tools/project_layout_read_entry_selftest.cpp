#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "core/Cell.h"
#include "core/Cut.h"
#include "core/Layer.h"
#include "core/Scene.h"
#include "io/ProjectLayoutReadEntry.h"
#include "io/ProjectLayoutSaveEntry.h"

namespace {

bool require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "selftest failed: " << message << '\n';
    }
    return condition;
}

void removeAllNoThrow(const std::filesystem::path& path)
{
    std::error_code error;
    std::filesystem::remove_all(path, error);
    if (error) {
        std::cerr << "cleanup warning: " << error.message() << '\n';
    }
}

std::filesystem::path uniqueSelftestRoot()
{
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("perapera_project_layout_read_entry_selftest_" + std::to_string(ticks));
}

perapera::Cell makeCell(std::string id, int frameCount, int layerCount)
{
    perapera::Cell cell;
    cell.id = std::move(id);
    cell.name = cell.id;
    cell.category = "character";

    for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        perapera::Frame frame;
        frame.name = "F" + std::to_string(frameIndex + 1);
        for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
            perapera::Layer layer;
            layer.layerId = "layer_" + std::to_string(frameIndex + 1) + "_" + std::to_string(layerIndex + 1);
            layer.name = "Layer " + std::to_string(layerIndex + 1);
            frame.layers.push_back(layer);
        }
        cell.frames.push_back(frame);
    }
    return cell;
}

} // namespace

int main()
{
    namespace fs = std::filesystem;

    const fs::path root = uniqueSelftestRoot();
    removeAllNoThrow(root);

    perapera::Scene scene;
    scene.id = "scene_001";
    scene.name = "Scene 1";

    perapera::Cut cut;
    cut.id = "cut_001";
    cut.name = "C001";
    cut.totalFrames = 12;
    cut.timesheet.totalFrames = 12;

    std::vector<perapera::Cell> cells;
    cells.push_back(makeCell("cell_A", 2, 2));
    cells.push_back(makeCell("cell_BG", 1, 1));

    std::string error;
    if (!require(perapera::saveProjectNewLayoutMinimal(root, scene, cut, cells, nullptr, &error),
                 "saveProjectNewLayoutMinimal failed")) {
        std::cerr << "error: " << error << '\n';
        removeAllNoThrow(root);
        return 1;
    }

    perapera::ProjectLayoutReadSummary summary;
    if (!require(perapera::inspectProjectNewLayoutMinimal(root, {}, &summary, &error),
                 "inspectProjectNewLayoutMinimal failed")) {
        std::cerr << "error: " << error << '\n';
        removeAllNoThrow(root);
        return 1;
    }

    if (!require(summary.cut.id == "cut_001", "wrong cut id") ||
        !require(summary.cut.totalFrames == 12, "wrong cut totalFrames") ||
        !require(summary.cellCount == 2, "wrong cell count") ||
        !require(summary.frameCount == 3, "wrong frame count") ||
        !require(summary.layerCount == 5, "wrong layer count") ||
        !require(summary.cellIds.size() == 2, "wrong cell id count") ||
        !require(summary.cellIds[0] == "cell_A", "wrong first cell id") ||
        !require(summary.cellIds[1] == "cell_BG", "wrong second cell id")) {
        removeAllNoThrow(root);
        return 1;
    }

    removeAllNoThrow(root);
    std::cout << "perapera_project_layout_read_entry_selftest passed\n";
    return 0;
}
