#include "io/ProjectLayoutLoadEntry.h"

#include <algorithm>
#include <array>
#include <cstdint>
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

bool readByteArray(const json& value, std::vector<std::uint8_t>& bytes, std::string* errorMessage) {
    if (!value.is_array()) {
        setError(errorMessage, "bitmap data is not an array");
        return false;
    }

    bytes.clear();
    bytes.reserve(value.size());
    for (const json& entry : value) {
        if (!entry.is_number_integer() && !entry.is_number_unsigned()) {
            setError(errorMessage, "bitmap data contains a non-integer value");
            return false;
        }
        const int byte = entry.get<int>();
        if (byte < 0 || byte > 255) {
            setError(errorMessage, "bitmap data value is outside 0..255");
            return false;
        }
        bytes.push_back(static_cast<std::uint8_t>(byte));
    }
    return true;
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

    stroke.brushEngine = strokeBrushEngineFromString(value.value("brushEngine", std::string{"Simple"}));
    const auto colorIterator = value.find("color");
    if (colorIterator != value.end()) {
        stroke.color = readColor(*colorIterator, stroke.color);
    }
    stroke.radiusPx = jsonFloat(value, "radiusPx", stroke.radiusPx);
    stroke.opacity = jsonFloat(value, "opacity", stroke.opacity);
    stroke.hardness = jsonFloat(value, "hardness", stroke.hardness);
    stroke.spacing = jsonFloat(value, "spacing", stroke.spacing);
    stroke.pressureToSize = jsonFloat(value, "pressureToSize", stroke.pressureToSize);
    stroke.pressureToOpacity = jsonFloat(value, "pressureToOpacity", stroke.pressureToOpacity);
    stroke.watercolorBleed = jsonFloat(value, "watercolorBleed", stroke.watercolorBleed);
    stroke.colorMix = jsonFloat(value, "colorMix", stroke.colorMix);
    stroke.dryRate = jsonFloat(value, "dryRate", stroke.dryRate);

    const auto pointsIterator = value.find("points");
    if (pointsIterator != value.end() && pointsIterator->is_array()) {
        for (const json& pointJson : *pointsIterator) {
            stroke.points.push_back(readStrokePoint(pointJson));
        }
    }

    ++strokeCount;
    return stroke;
}

bool readStrokeBitmap(const json& value, Stroke& stroke, const fs::path& layerPath, std::string* errorMessage) {
    if (stroke.brushEngine != StrokeBrushEngine::Fill) {
        return true;
    }

    const auto bitmapIterator = value.find("bitmap");
    if (bitmapIterator == value.end() || !bitmapIterator->is_object()) {
        setError(errorMessage, "fill stroke is missing bitmap: " + layerPath.string());
        return false;
    }

    const json& bitmapJson = *bitmapIterator;
    stroke.bitmapX = jsonInt(bitmapJson, "x", stroke.bitmapX);
    stroke.bitmapY = jsonInt(bitmapJson, "y", stroke.bitmapY);
    stroke.bitmapWidth = jsonInt(bitmapJson, "width", stroke.bitmapWidth);
    stroke.bitmapHeight = jsonInt(bitmapJson, "height", stroke.bitmapHeight);

    const auto dataIterator = bitmapJson.find("data");
    if (dataIterator == bitmapJson.end() || !readByteArray(*dataIterator, stroke.bitmap, errorMessage)) {
        setError(errorMessage, "failed to read fill bitmap data: " + layerPath.string());
        return false;
    }

    const std::size_t expectedSize = static_cast<std::size_t>(std::max(0, stroke.bitmapWidth)) *
                                     static_cast<std::size_t>(std::max(0, stroke.bitmapHeight));
    if (expectedSize != stroke.bitmap.size()) {
        setError(errorMessage, "fill bitmap size mismatch: " + layerPath.string());
        return false;
    }

    return true;
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
    layer.type = layerTypeFromString(layerJson.value("type", std::string{"Normal"}));
    layer.strokes.clear();

    const json strokesJson = layerJson.value("strokes", json::array());
    if (!strokesJson.is_array()) {
        setError(errorMessage, "layer strokes is not an array: " + layerPath.string());
        return false;
    }

    for (const json& strokeJson : strokesJson) {
        Stroke stroke = readStroke(strokeJson, strokeCount);
        if (!readStrokeBitmap(strokeJson, stroke, layerPath, errorMessage)) {
            return false;
        }
        layer.strokes.push_back(std::move(stroke));
    }
    return true;
}

void appendLoadedCell(Project& project, std::vector<Cell>& cells, std::vector<bool>& consumed, const std::string& cellId) {
    for (std::size_t index = 0; index < cells.size(); ++index) {
        if (!consumed[index] && cells[index].id == cellId) {
            project.cellOrder.push_back(cells[index].id);
            project.cells.push_back(std::move(cells[index]));
            consumed[index] = true;
            return;
        }
    }
}

void applyCutCellOrder(Project& project, std::vector<Cell>& loadedCells, const std::vector<std::string>& cutCellOrder) {
    project.cells.clear();
    project.cellOrder.clear();

    std::vector<bool> consumed(loadedCells.size(), false);
    for (const std::string& cellId : cutCellOrder) {
        appendLoadedCell(project, loadedCells, consumed, cellId);
    }

    for (std::size_t index = 0; index < loadedCells.size(); ++index) {
        if (!consumed[index]) {
            project.cellOrder.push_back(loadedCells[index].id);
            project.cells.push_back(std::move(loadedCells[index]));
            consumed[index] = true;
        }
    }
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
    result.project.name = result.scene.name.empty() ? result.project.name : result.scene.name;
    result.project.timeline.totalFrames = result.cut.totalFrames > 0 ? result.cut.totalFrames : result.project.timeline.totalFrames;
    result.project.output.fps = result.cut.frameRate > 0 ? result.cut.frameRate : result.project.output.fps;
    result.project.cells.clear();
    result.project.cellOrder.clear();

    std::vector<Cell> loadedCells;
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
        loadedCells.push_back(std::move(cell));
    }

    applyCutCellOrder(result.project, loadedCells, result.cut.cellZOrderKeys);
    result.cellCount = result.project.cells.size();
    *outResult = std::move(result);
    return true;
}

} // namespace perapera
