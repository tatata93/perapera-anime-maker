#include "io/ProjectLayoutSaveEntry.h"

#include "io/CellLayoutIO.h"
#include "io/LayerLayoutIO.h"
#include "io/ProjectLayoutPaths.h"
#include "io/SceneCutLayoutIO.h"

namespace perapera {
namespace {

void setError(std::string* errorMessage, const std::string& text)
{
    if (errorMessage != nullptr) {
        *errorMessage = text;
    }
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
        outResult->sceneJsonPath = sceneCutResult.sceneJsonPath;
        outResult->cutJsonPath = sceneCutResult.cutJsonPath;
        outResult->timesheetJsonPath = sceneCutResult.timesheetJsonPath;
        outResult->cellCount = cells.size();
        outResult->frameCount = totalFrames;
    }

    return true;
}

} // namespace perapera
