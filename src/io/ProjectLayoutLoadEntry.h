#pragma once
// final_spec_v6 Phase 2 Step 2-h.
// Load the new scene/cut/cell/frame/layer layout into the current Project model.
// Legacy ProjectIO compatibility is intentionally not handled here.

#include <cstddef>
#include <filesystem>
#include <string>

#include "core/Cut.h"
#include "core/Project.h"
#include "core/Scene.h"

namespace perapera {

struct ProjectLayoutLoadOptions {
    std::string sceneId = "scene_001";
    std::string cutId = "cut_001";
    bool requireLayerJson = true;
};

struct ProjectLayoutLoadResult {
    Scene scene;
    Cut cut;
    Project project;
    std::size_t cellCount = 0;
    std::size_t frameCount = 0;
    std::size_t layerCount = 0;
    std::size_t strokeCount = 0;
};

bool loadProjectNewLayoutMinimal(
    const std::filesystem::path& projectRoot,
    const ProjectLayoutLoadOptions& options,
    ProjectLayoutLoadResult* outResult,
    std::string* errorMessage = nullptr);

} // namespace perapera
