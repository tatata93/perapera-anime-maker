#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/Cell.h"

namespace perapera {

struct CellLayoutSaveOptions {
    std::string sceneId = "scene_001";
    std::string cutId = "cut_001";
    bool writeFrameManifests = true;
};

struct CellLayoutSaveSummary {
    std::filesystem::path cellDirectory;
    std::filesystem::path cellJson;
    std::filesystem::path framesDirectory;
    std::size_t frameCount = 0;
};

bool saveCellLayoutMinimal(const std::filesystem::path& projectRoot,
                           const Cell& cell,
                           const CellLayoutSaveOptions& options,
                           CellLayoutSaveSummary* outSummary = nullptr);

bool saveCellsLayoutMinimal(const std::filesystem::path& projectRoot,
                            const std::vector<Cell>& cells,
                            const CellLayoutSaveOptions& options,
                            std::vector<CellLayoutSaveSummary>* outSummaries = nullptr);

} // namespace perapera
