#include "io/ProjectLayoutReadEntry.h"

#include "io/CutIO.h"
#include "io/ProjectLayoutPaths.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <system_error>

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

bool readJsonFile(const fs::path& path, json& value, std::string* errorMessage)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        setError(errorMessage, "failed to open json: " + path.string());
        return false;
    }

    try {
        input >> value;
    } catch (const std::exception& exception) {
        setError(errorMessage, "failed to parse json: " + path.string() + " / " + exception.what());
        return false;
    }

    if (!value.is_object()) {
        setError(errorMessage, "json root is not an object: " + path.string());
        return false;
    }

    return true;
}

std::vector<fs::path> sortedDirectories(const fs::path& root)
{
    std::vector<fs::path> result;
    std::error_code errorCode;
    if (!fs::is_directory(root, errorCode)) {
        return result;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(root, errorCode)) {
        if (!errorCode && entry.is_directory()) {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<fs::path> sortedJsonFiles(const fs::path& root)
{
    std::vector<fs::path> result;
    std::error_code errorCode;
    if (!fs::is_directory(root, errorCode)) {
        return result;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(root, errorCode)) {
        if (!errorCode && entry.is_regular_file() && entry.path().extension() == ".json") {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool inspectFrame(const fs::path& cellDir,
                  const json& frameEntry,
                  bool requireLayerJson,
                  std::size_t& frameCount,
                  std::size_t& layerCount,
                  std::string* errorMessage)
{
    const std::string framePathText = frameEntry.value("path", "");
    if (framePathText.empty()) {
        setError(errorMessage, "frame entry is missing path in: " + cellDir.string());
        return false;
    }

    const fs::path frameJsonPath = cellDir / fs::path(framePathText);
    json frameJson;
    if (!readJsonFile(frameJsonPath, frameJson, errorMessage)) {
        return false;
    }

    ++frameCount;

    const std::size_t expectedLayerCount = frameJson.value("layerCount", 0U);
    const std::string layersDirectoryName = frameJson.value("layersDirectory", "layers");
    const fs::path layersDirectory = frameJsonPath.parent_path() / layersDirectoryName;
    const std::vector<fs::path> layerFiles = sortedJsonFiles(layersDirectory);

    if (requireLayerJson && layerFiles.size() < expectedLayerCount) {
        setError(errorMessage, "not enough layer json files in: " + layersDirectory.string());
        return false;
    }

    for (const fs::path& layerPath : layerFiles) {
        json layerJson;
        if (!readJsonFile(layerPath, layerJson, errorMessage)) {
            return false;
        }
        if (layerJson.value("schema", "") != "perapera.layer.v1") {
            setError(errorMessage, "wrong layer schema: " + layerPath.string());
            return false;
        }
        ++layerCount;
    }

    return true;
}

bool inspectCell(const fs::path& cellDir,
                 bool requireLayerJson,
                 ProjectLayoutReadSummary& summary,
                 std::string* errorMessage)
{
    const fs::path cellJsonPath = cellDir / "cell.json";
    json cellJson;
    if (!readJsonFile(cellJsonPath, cellJson, errorMessage)) {
        return false;
    }

    const std::string cellId = cellJson.value("id", cellDir.filename().string());
    if (cellId.empty()) {
        setError(errorMessage, "cell id is empty: " + cellJsonPath.string());
        return false;
    }

    const json frames = cellJson.value("frames", json::array());
    if (!frames.is_array()) {
        setError(errorMessage, "cell frames is not an array: " + cellJsonPath.string());
        return false;
    }

    summary.cellIds.push_back(cellId);
    ++summary.cellCount;

    for (const json& frameEntry : frames) {
        if (!frameEntry.is_object()) {
            setError(errorMessage, "frame entry is not an object: " + cellJsonPath.string());
            return false;
        }
        if (!inspectFrame(cellDir, frameEntry, requireLayerJson, summary.frameCount, summary.layerCount, errorMessage)) {
            return false;
        }
    }

    return true;
}

} // namespace

bool inspectProjectNewLayoutMinimal(const std::filesystem::path& projectRoot,
                                    const ProjectLayoutReadOptions& options,
                                    ProjectLayoutReadSummary* outSummary,
                                    std::string* errorMessage)
{
    const std::string sceneId = normalizeLayoutId(options.sceneId, defaultSceneId());
    const std::string cutId = normalizeLayoutId(options.cutId, defaultCutId());

    ProjectLayoutReadSummary summary;
    summary.sceneJsonPath = sceneJsonPath(projectRoot, sceneId);
    summary.cutFolder = cutDirectory(projectRoot, sceneId, cutId);
    summary.cutJsonPath = CutIO::cutJsonPathForCutFolder(summary.cutFolder);
    summary.timesheetJsonPath = CutIO::timesheetJsonPathForCutFolder(summary.cutFolder);

    json sceneJson;
    if (!readJsonFile(summary.sceneJsonPath, sceneJson, errorMessage)) {
        return false;
    }
    if (sceneJson.value("kind", "") != "perapera.scene.v1") {
        setError(errorMessage, "wrong scene kind: " + summary.sceneJsonPath.string());
        return false;
    }
    if (sceneJson.value("id", "") != sceneId) {
        setError(errorMessage, "scene id mismatch: " + summary.sceneJsonPath.string());
        return false;
    }

    if (!CutIO::loadCut(summary.cutFolder, summary.cut, errorMessage)) {
        return false;
    }
    if (summary.cut.id != cutId) {
        setError(errorMessage, "cut id mismatch: " + summary.cutJsonPath.string());
        return false;
    }

    const fs::path cellsDir = cellsDirectory(projectRoot, sceneId, cutId);
    const std::vector<fs::path> cellDirs = sortedDirectories(cellsDir);
    for (const fs::path& cellDir : cellDirs) {
        if (!inspectCell(cellDir, options.requireLayerJson, summary, errorMessage)) {
            return false;
        }
    }

    if (outSummary != nullptr) {
        *outSummary = std::move(summary);
    }
    return true;
}

} // namespace perapera
