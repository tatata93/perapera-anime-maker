#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/Cell.h"
#include "core/Cut.h"
#include "core/Layer.h"
#include "core/Scene.h"
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
           ("perapera_project_layout_save_entry_selftest_" + std::to_string(ticks));
}

perapera::Cell makeMinimalCell()
{
    perapera::Cell cell;
    cell.id = "cell_A";
    cell.name = "Cell A";
    cell.category = "character";

    perapera::Frame frame;
    frame.name = "F001";

    perapera::Layer layer;
    layer.name = "line";
    frame.layers.push_back(layer);

    cell.frames.push_back(frame);
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

    std::vector<perapera::Cell> cells;
    cells.push_back(makeMinimalCell());

    perapera::ProjectLayoutSaveEntryResult result;
    std::string errorMessage;
    if (!require(perapera::saveProjectNewLayoutMinimal(root, scene, cut, cells, &result, &errorMessage),
                 "saveProjectNewLayoutMinimal failed")) {
        std::cerr << "error: " << errorMessage << '\n';
        removeAllNoThrow(root);
        return 1;
    }

    const fs::path cutDir = root / "scenes" / "scene_001" / "cuts" / "cut_001";
    const fs::path expected[] = {
        root / "scenes" / "scene_001" / "scene.json",
        cutDir / "cut.json",
        cutDir / "timesheet.json",
        cutDir / "cells" / "cell_A" / "cell.json",
        cutDir / "cells" / "cell_A" / "frames" / "frame_001" / "frame.json",
        cutDir / "cells" / "cell_A" / "frames" / "frame_001" / "layers" / "layer_001.json",
    };

    for (const fs::path& path : expected) {
        if (!require(fs::exists(path), path.string().c_str())) {
            removeAllNoThrow(root);
            return 1;
        }
    }

    if (!require(result.cellCount == 1, "wrong cellCount") ||
        !require(result.frameCount == 1, "wrong frameCount")) {
        removeAllNoThrow(root);
        return 1;
    }

    nlohmann::json layerJson;
    {
        std::ifstream input(expected[5]);
        if (!require(static_cast<bool>(input), "failed to open layer_001.json")) {
            removeAllNoThrow(root);
            return 1;
        }
        try {
            input >> layerJson;
        } catch (const std::exception& error) {
            std::cerr << "failed to parse layer_001.json: " << error.what() << '\n';
            removeAllNoThrow(root);
            return 1;
        }
    }

    if (!require(layerJson.value("schema", "") == "perapera.layer.v1", "wrong layer schema")) {
        removeAllNoThrow(root);
        return 1;
    }

    removeAllNoThrow(root);
    std::cout << "perapera_project_layout_save_entry_selftest passed\n";
    return 0;
}
