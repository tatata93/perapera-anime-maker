#include "io/ProjectLayoutSaveEntry.h"

#include "io/CellLayoutIO.h"
#include "io/LayerLayoutIO.h"
#include "io/ProjectLayoutPaths.h"
#include "io/SceneCutLayoutIO.h"

#include <fstream>
#include <sstream>
#include <system_error>

#include <nlohmann/json.hpp>

namespace perapera {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

void setError(std::string* errorMessage, const std::string& text)
{
    if (errorMessage != nullptr) {
        *errorMessage = text;
    }
}

json projectMetadataToJson(const Project& project, const Scene& scene)
{
    json sceneOrder = json::array();
    sceneOrder.push_back(normalizeLayoutId(scene.id, defaultSceneId()));

    json cameraKeys = json::array();
    for (const CameraKey& key : project.camera.keys) {
        cameraKeys.push_back({
            {"frame", key.frame},
            {"centerX", key.centerX},
            {"centerY", key.centerY},
            {"zoom", key.zoom},
        });
    }

    return json{
        {"kind", "perapera.project.v1"},
        {"formatVersion", 1},
        {"format_version", 1},
        {"name", project.name},
        {"canvas", {{"width", project.canvas.width}, {"height", project.canvas.height}}},
        {"output",
         {{"width", project.output.width},
          {"height", project.output.height},
          {"fps", project.output.fps},
          {"pixelAspect", project.output.pixelAspect}}},
        {"audio",
         {{"enabled", project.audio.enabled},
          {"filePath", project.audio.filePath},
          {"startFrame", project.audio.startFrame}}},
        {"camera",
         {{"centerX", project.camera.centerX},
          {"centerY", project.camera.centerY},
          {"zoom", project.camera.zoom},
          {"animationEnabled", project.camera.animationEnabled},
          {"keys", std::move(cameraKeys)}}},
        {"sceneOrder", std::move(sceneOrder)},
    };
}

bool writeJsonFileIfChanged(const fs::path& path, const json& value, std::string* errorMessage)
{
    const std::string text = value.dump(4) + '\n';

    std::ifstream existing(path, std::ios::binary);
    if (existing) {
        std::ostringstream buffer;
        buffer << existing.rdbuf();
        if (buffer.str() == text) {
            return true;
        }
    }

    const fs::path parent = path.parent_path();
    if (!parent.empty()) {
        std::error_code errorCode;
        fs::create_directories(parent, errorCode);
        if (errorCode) {
            setError(errorMessage, "failed to create project json folder: " + parent.string());
            return false;
        }
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        setError(errorMessage, "failed to open project json for write: " + path.string());
        return false;
    }

    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!output) {
        setError(errorMessage, "failed to write project json: " + path.string());
        return false;
    }

    return true;
}

bool saveProjectMetadataJson(const fs::path& projectRoot, const Scene& scene, const Project& project, std::string* errorMessage)
{
    return writeJsonFileIfChanged(projectJsonPath(projectRoot), projectMetadataToJson(project, scene), errorMessage);
}

} // namespace

bool saveProjectNewLayoutMinimal(const std::filesystem::path& projectRoot,
                                 const Scene& scene,
                                 const Cut& cut,
                                 const std::vector<Cell>& cells,
                                 ProjectLayoutSaveEntryResult* outResult,
                                 std::string* errorMessage)
{
    SceneCutLayoutIO::SceneCutLayoutSaveResult sceneCutResult;
    if (!SceneCutLayoutIO::saveMinimalSceneAndCut(projectRoot, scene, cut, &sceneCutResult, errorMessage)) {
        return false;
    }

    CellLayoutSaveOptions cellOptions;
    cellOptions.sceneId = scene.id;
    cellOptions.cutId = cut.id;
    cellOptions.writeFrameManifests = true;

    std::vector<CellLayoutSaveSummary> cellSummaries;
    if (!saveCellsLayoutMinimal(projectRoot, cells, cellOptions, &cellSummaries)) {
        setError(errorMessage, "saveCellsLayoutMinimal failed");
        return false;
    }

    const std::string sceneId = normalizeLayoutId(scene.id, defaultSceneId());
    const std::string cutId = normalizeLayoutId(cut.id, defaultCutId());

    std::size_t totalFrames = 0;
    for (const Cell& cell : cells) {
        const std::string cellId = normalizeLayoutId(cell.id, "cell_001");
        for (std::size_t index = 0; index < cell.frames.size(); ++index) {
            const int frameIndex = static_cast<int>(index);
            const std::filesystem::path frameDir = frameDirectory(projectRoot, sceneId, cutId, cellId, frameIndex);
            if (!saveFrameLayersLayout(frameDir, cell.frames[index])) {
                setError(errorMessage, "saveFrameLayersLayout failed: " + frameDir.string());
                return false;
            }
            ++totalFrames;
        }
    }

    if (outResult != nullptr) {
        outResult->projectJsonPath.clear();
        outResult->sceneJsonPath = sceneCutResult.sceneJsonPath;
        outResult->cutJsonPath = sceneCutResult.cutJsonPath;
        outResult->timesheetJsonPath = sceneCutResult.timesheetJsonPath;
        outResult->cellCount = cells.size();
        outResult->frameCount = totalFrames;
    }

    return true;
}

bool saveProjectNewLayoutMinimal(const std::filesystem::path& projectRoot,
                                 const Scene& scene,
                                 const Cut& cut,
                                 const Project& project,
                                 ProjectLayoutSaveEntryResult* outResult,
                                 std::string* errorMessage)
{
    if (!saveProjectMetadataJson(projectRoot, scene, project, errorMessage)) {
        return false;
    }

    if (!saveProjectNewLayoutMinimal(projectRoot, scene, cut, project.cells, outResult, errorMessage)) {
        return false;
    }

    if (outResult != nullptr) {
        outResult->projectJsonPath = projectJsonPath(projectRoot);
    }
    return true;
}

} // namespace perapera
