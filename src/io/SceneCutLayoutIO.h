#pragma once

// final_spec_v6 Phase 2 Step 2-b.
// Minimal writer for the future Project -> Scene -> Cut folder layout.
// This writes the scene/cut portion of the active new layout.

#include <filesystem>
#include <string>

#include "core/Cut.h"
#include "core/Scene.h"

namespace perapera::SceneCutLayoutIO {

struct SceneCutLayoutSaveResult {
    std::filesystem::path sceneJsonPath;
    std::filesystem::path cutFolder;
    std::filesystem::path cutJsonPath;
    std::filesystem::path timesheetJsonPath;
};

bool saveMinimalSceneAndCut(
    const std::filesystem::path& projectRoot,
    const Scene& scene,
    const Cut& cut,
    SceneCutLayoutSaveResult* result = nullptr,
    std::string* errorMessage = nullptr);

} // namespace perapera::SceneCutLayoutIO
