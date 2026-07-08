#pragma once

// final_spec_v6 Phase 2 Step 2-e.
// One small entry point that assembles the new scene/cut/cell/frame/layer save layout.
// This does not replace UI save/load yet.

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "core/Cell.h"
#include "core/Cut.h"
#include "core/Project.h"
#include "core/Scene.h"

namespace perapera {

struct ProjectLayoutSaveEntryResult {
    std::filesystem::path projectJsonPath;
    std::filesystem::path sceneJsonPath;
    std::filesystem::path cutJsonPath;
    std::filesystem::path timesheetJsonPath;
    std::size_t cellCount = 0;
    std::size_t frameCount = 0;
};

bool saveProjectNewLayoutMinimal(const std::filesystem::path& projectRoot,
                                 const Scene& scene,
                                 const Cut& cut,
                                 const std::vector<Cell>& cells,
                                 ProjectLayoutSaveEntryResult* outResult = nullptr,
                                 std::string* errorMessage = nullptr);

bool saveProjectNewLayoutMinimal(const std::filesystem::path& projectRoot,
                                 const Scene& scene,
                                 const Cut& cut,
                                 const Project& project,
                                 ProjectLayoutSaveEntryResult* outResult = nullptr,
                                 std::string* errorMessage = nullptr);

} // namespace perapera
