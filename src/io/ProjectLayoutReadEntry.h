#pragma once

// final_spec_v6 Phase 2 Step 2-g.
// Narrow reader/inspector for the new project layout. This verifies the saved
// structure before the app load path is replaced.

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "core/Cut.h"

namespace perapera {

struct ProjectLayoutReadOptions {
    std::string sceneId = "scene_001";
    std::string cutId = "cut_001";
    bool requireLayerJson = true;
};

struct ProjectLayoutReadSummary {
    std::filesystem::path sceneJsonPath;
    std::filesystem::path cutFolder;
    std::filesystem::path cutJsonPath;
    std::filesystem::path timesheetJsonPath;
    Cut cut;
    std::vector<std::string> cellIds;
    std::size_t cellCount = 0;
    std::size_t frameCount = 0;
    std::size_t layerCount = 0;
};

bool inspectProjectNewLayoutMinimal(const std::filesystem::path& projectRoot,
                                    const ProjectLayoutReadOptions& options,
                                    ProjectLayoutReadSummary* outSummary = nullptr,
                                    std::string* errorMessage = nullptr);

} // namespace perapera
