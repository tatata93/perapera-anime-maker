#include "io/ProjectLayoutLoadEntry.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/StrokePoint.h"
#include "io/CutIO.h"
#include "io/ProjectLayoutPaths.h"

namespace perapera {
namespace {
namespace fs = std::filesystem;
using nlohmann::json;

void setError(std::string* errorMessage, const std::string& text) {
    if (errorMessage != nullptr) {
        *errorMessage = text;
    }
}

bool readJsonFile(const fs::path& path, json& value, std::string* errorMessage) {
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

std::vector<fs::path> sortedDirectories(const fs::path& root) {
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

std::vector<fs::path> sortedJsonFiles(const fs::path& root) {
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

float jsonFloat(const json& value, const char* key, float fallback) {
    const auto iterator = value.find(key);
    if (iterator == value.end() || !iterator->is_number()) {
        return fallback;
    }
    return iterator->get<float>();
}

int jsonInt(const json& value, const char* key, int fallback) {
    const auto iterator = value.find(key);
    if (iterator == value.end() || !iterator->is_number_integer()) {
        return fallback;
    }
    return iterator->get<int>();
}

std::array<float, 4> readColor(const json& value, const std::array<float, 4>& fallback) {
    if (!value.is_array() || value.size() < 4) {
        return fallback;
    }
    std::array<float, 4> color = fallback;
    for (std::size_t index = 0; index < color.size(); ++index) {
        if (value[index].is_number()) {
            color[index] = value[index].get<float>();
        }
    }
    return color;
}

StrokePoint readStrokePoint(const json& value) {
    StrokePoint point;
    if (value.is_object()) {
        point.x = jsonFloat(value, "x", point.x);
        point.y = jsonFloat(value, "y", point.y);
        point.pressure = jsonFloat(value, "pressure", point.pressure);
    }
    return point;
}

Stroke readStroke(const json& value, std::size_t& strokeCount) {
    Stroke stroke;
    if (!value.is_object()) {
        return stroke;
    }

    const auto colorIterator = value.find("color");
    if (colorIterator != value.end()) {
        stroke.color = readColor(*colorIterator, stroke.color);
    }
    stroke.radiusPx = jsonFloat(value, "radiusPx", stroke.radiusPx);

    const auto pointsIterator = value.find("points");
    if (pointsIterator != value.end() && pointsIterator->is_array()) {
        for (const json& pointJson : *pointsIterator) {
            stroke.points.push_back(readStrokePoint(pointJson));
        }
    }

    ++strokeCount;
    return stroke;
}

bool readLayer(const fs::path& layerPath, Layer& layer, std::size_t& strokeCount, std::string* errorMessage) {
    json layerJson;
    if (!readJsonFile(layerPath, layerJson, errorMessage)) {
        return false;
    }

    if (layerJson.value("schema", "") != "perapera.layer.v1") {
        setError(errorMessage, "wrong layer schema: " + layerPath.string());
        return false;
    }

    layer.layerId = layerJson.value("layerId", layer.layerId.empty() ? layerPath.stem().string() : layer.layerId);
    layer.name = layerJson.value("name", layer.name);
    layer.visible = layerJson.value("visible", layer.visible);
    layer.opacity = jsonFloat(layerJson, "opacity", layer.opacity);
    layer.blendMode = layerJson.value("blendMode", layer.blendMode);
    layer.strokes.clear();

    const json strokesJson = layerJson.value("strokes", json::array());
    if (!strokesJson.is_array()) {
        setError(errorMessage, "layer strokes is not an array: " + layerPath.string());
        return false;
    }

    for (const json& strokeJson : strokesJson) {
        layer.strokes.push_back(readStroke(strokeJson, strokeCount));
    }
    return true;
}

bool readFrame(
    const fs::path& cellDir,
    const json& frameEntry,
    bool requireLayerJson,
    Frame& frame,
    std::size_t& layerCount,
    std::size_t& strokeCount,
    std::string* errorMessage) {
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

    frame.name = frameJson.value("name", frameEntry.value("name", frame.name));
    frame.durationFrames = jsonInt(frameJson, "durationFrames", frameEntry.value("durationFrames", frame.durationFrames));
    frame.layers.clear();

    const std::size_t expectedLayerCount = frameJson.value("layerCount", 0U);
    const std::string layersDirectoryName = frameJson.value("layersDirectory", "layers");
    const fs::path layersDirectory = frameJsonPath.parent_path() / layersDirectoryName;
    const std::vector<fs::path> layerFiles = sortedJsonFiles(layersDirectory);

    if (requireLayerJson && layerFiles.size() < expectedLayerCount) {
        setError(errorMessage, "not enough layer json files in: " + layersDirectory.string());
        return false;
    }

    for (const fs::path& layerPath : layerFiles) {
        Layer layer;
        if (!readLayer(layerPath, layer, strokeCount, errorMessage)) {
            return false;
        }
        frame.layers.push_back(std::move(layer));
        ++layerCount;
    }

    return true;
}

bool readCell(
    const fs::path& cellDir,
    bool requireLayerJson,
    Cell& cell,
    std::size_t& frameCount,
    std::size_t& layerCount,
    std::size_t& strokeCount,
    std::string* errorMessage) {
    const fs::path cellJsonPath = cellDir / "cell.json";
    json cellJson;
    if (!readJsonFile(cellJsonPath, cellJson, errorMessage)) {
        return false;
    }

    cell.id = cellJson.value("id", cellDir.filename().string());
    cell.name = cellJson.value("name", cell.name);
    cell.category = cellJson.value("category", cell.category);
    cell.widthPx = jsonInt(cellJson, "widthPx", cell.widthPx);
    cell.heightPx = jsonInt(cellJson, "heightPx", cell.heightPx);
    cell.zOrder = jsonInt(cellJson, "zOrder", cell.zOrder);
    cell.visible = cellJson.value("visible", cell.visible);
    cell.opacity = jsonFloat(cellJson, "opacity", cell.opacity);

    const auto placementIterator = cellJson.find("placement");
    if (placementIterator != cellJson.end() && placementIterator->is_object()) {
        cell.placement.x = jsonFloat(*placementIterator, "x", cell.placement.x);
        cell.placement.y = jsonFloat(*placementIterator, "y", cell.placement.y);
        cell.placement.scale = jsonFloat(*placementIterator, "scale", cell.placement.scale);
        cell.placement.rotation = jsonFloat(*placementIterator, "rotation", cell.placement.rotation);
    }

    const json framesJson = cellJson.value("frames", json::array());
    if (!framesJson.is_array()) {
        setError(errorMessage, "cell frames is not an array: " + cellJsonPath.string());
        return false;
    }

    cell.frames.clear();
    for (const json& frameEntry : framesJson) {
        if (!frameEntry.is_object()) {
            setError(errorMessage, "frame entry is not an object: " + cellJsonPath.string());
            return false;
        }
        Frame frame;
        if (!readFrame(cellDir, frameEntry, requireLayerJson, frame, layerCount, strokeCount, errorMessage)) {
            return false;
        }
        cell.frames.push_back(std::move(frame));
        ++frameCount;
    }

    return true;
}

bool readSceneJson(const fs::path& scenePath, const std::string& sceneId, Scene& scene, std::string* errorMessage) {
    json sceneJson;
    if (!readJsonFile(scenePath, sceneJson, errorMessage)) {
        return false;
    }
    if (sceneJson.value("kind", "") != "perapera.scene.v1") {
        setError(errorMessage, "wrong scene kind: " + scenePath.string());
        return false;
    }

    scene.id = sceneJson.value("id", sceneId);
    scene.name = sceneJson.value("name", scene.id);
    scene.memo = sceneJson.value("memo", std::string{});
    scene.cutOrder.clear();

    const json cutOrder = sceneJson.value("cutOrder", json::array());
    if (cutOrder.is_array()) {
        for (const json& entry : cutOrder) {
            if (entry.is_string()) {
                scene.cutOrder.push_back(entry.get<std::string>());
            }
        }
    }
    return true;
}

} // namespace

bool loadProjectNewLayoutMinimal(
    const std::filesystem::path& projectRoot,
    const ProjectLayoutLoadOptions& options,
    ProjectLayoutLoadResult* outResult,
    std::string* errorMessage) {
    if (outResult == nullptr) {
        setError(errorMessage, "outResult is null");
        return false;
    }

    const std::string sceneId = normalizeLayoutId(options.sceneId, defaultSceneId());
    const std::string cutId = normalizeLayoutId(options.cutId, defaultCutId());

    ProjectLayoutLoadResult result;
    if (!readSceneJson(sceneJsonPath(projectRoot, sceneId), sceneId, result.scene, errorMessage)) {
        return false;
    }

    const fs::path cutFolder = cutDirectory(projectRoot, sceneId, cutId);
    if (!CutIO::loadCut(cutFolder, result.cut, errorMessage)) {
        return false;
    }
    result.scene.cuts.clear();
    result.scene.cuts.push_back(result.cut);

    result.project = Project{};
    result.project.cells.clear();
    result.project.cellOrder.clear();

    const fs::path cellsDir = cellsDirectory(projectRoot, sceneId, cutId);
    for (const fs::path& cellDir : sortedDirectories(cellsDir)) {
        Cell cell;
        if (!readCell(
                cellDir,
                options.requireLayerJson,
                cell,
                result.frameCount,
                result.layerCount,
                result.strokeCount,
                errorMessage)) {
            return false;
        }
        result.project.cellOrder.push_back(cell.id);
        result.project.cells.push_back(std::move(cell));
    }

    result.cellCount = result.project.cells.size();
    *outResult = std::move(result);
    return true;
}

} // namespace perapera
