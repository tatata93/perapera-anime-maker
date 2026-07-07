#pragma once

#include "drawing/DrawingCell.h"

#include <filesystem>
#include <string>
#include <vector>

namespace perapera {

struct DrawingNewLayoutIOOptions {
    std::string sceneId = "scene_001";
    std::string cutId = "cut_001";
    bool requireLayerJson = true;
};

struct DrawingNewLayoutSaveResult {
    std::filesystem::path cellsDirectory;
    std::size_t cellCount = 0;
    std::size_t frameCount = 0;
    std::size_t layerCount = 0;
    std::size_t strokeCount = 0;
};

struct DrawingNewLayoutLoadResult {
    std::filesystem::path cellsDirectory;
    std::vector<DrawingCell> cells;
    std::size_t frameCount = 0;
    std::size_t layerCount = 0;
    std::size_t strokeCount = 0;
};

bool saveDrawingCellsToNewLayout(
    const std::filesystem::path& projectRoot,
    const DrawingNewLayoutIOOptions& options,
    const std::vector<DrawingCell>& cells,
    DrawingNewLayoutSaveResult* outResult,
    std::string* errorMessage);

bool loadDrawingCellsFromNewLayout(
    const std::filesystem::path& projectRoot,
    const DrawingNewLayoutIOOptions& options,
    DrawingNewLayoutLoadResult* outResult,
    std::string* errorMessage);

} // namespace perapera
