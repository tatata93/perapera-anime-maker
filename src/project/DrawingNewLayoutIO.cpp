#include "project/DrawingNewLayoutIO.h"

#include "io/ProjectLayoutPaths.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
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

std::string paddedName(const char* prefix, int zeroBasedIndex)
{
    std::ostringstream stream;
    stream << prefix << std::setw(3) << std::setfill('0') << (zeroBasedIndex + 1);
    return stream.str();
}

const char* categoryToText(DrawingCellCategory category)
{
    switch (category) {
    case DrawingCellCategory::Background:
        return "background";
    case DrawingCellCategory::Character:
        return "character";
    case DrawingCellCategory::Book:
        return "book";
    case DrawingCellCategory::Effect:
        return "effect";
    case DrawingCellCategory::Other:
        return "other";
    }
    return "other";
}

DrawingCellCategory categoryFromText(const std::string& text)
{
    if (text == "background") {
        return DrawingCellCategory::Background;
    }
    if (text == "character") {
        return DrawingCellCategory::Character;
    }
    if (text == "book") {
        return DrawingCellCategory::Book;
    }
    if (text == "effect") {
        return DrawingCellCategory::Effect;
    }
    return DrawingCellCategory::Other;
}

json strokeToJson(const Stroke& stroke)
{
    json points = json::array();
    for (const CanvasPoint& point : stroke.points) {
        points.push_back({
            {"x", point.x},
            {"y", point.y},
            {"pressure", 1.0f},
        });
    }

    return json{
        {"color", json::array({stroke.color.r, stroke.color.g, stroke.color.b, stroke.color.a})},
        {"radiusPx", stroke.radiusPx},
        {"points", points},
    };
}

Stroke strokeFromJson(const json& value)
{
    Stroke stroke;
    if (!value.is_object()) {
        return stroke;
    }

    const json color = value.value("color", json::array());
    if (color.is_array() && color.size() >= 4) {
        if (color[0].is_number()) {
            stroke.color.r = color[0].get<float>();
        }
        if (color[1].is_number()) {
            stroke.color.g = color[1].get<float>();
        }
        if (color[2].is_number()) {
            stroke.color.b = color[2].get<float>();
        }
        if (color[3].is_number()) {
            stroke.color.a = color[3].get<float>();
        }
    }

    stroke.radiusPx = value.value("radiusPx", stroke.radiusPx);

    const json points = value.value("points", json::array());
    if (points.is_array()) {
        for (const json& pointJson : points) {
            if (!pointJson.is_object()) {
                continue;
            }
            CanvasPoint point;
            point.x = pointJson.value("x", 0.0f);
            point.y = pointJson.value("y", 0.0f);
            stroke.points.push_back(point);
        }
    }

    stroke.invalidateBounds();
    return stroke;
}

bool writeJsonFile(const fs::path& path, const json& value, std::string* errorMessage)
{
    std::error_code directoryError;
    fs::create_directories(path.parent_path(), directoryError);
    if (directoryError) {
        setError(errorMessage, "failed to create directory: " + path.parent_path().string());
        return false;
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        setError(errorMessage, "failed to open json for write: " + path.string());
        return false;
    }

    output << value.dump(2) << '\n';
    if (!output) {
        setError(errorMessage, "failed to write json: " + path.string());
        return false;
    }
    return true;
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

json layerToJson(const DrawingLayer& layer, int layerIndex)
{
    json strokes = json::array();
    for (const Stroke& stroke : layer.strokes) {
        strokes.push_back(strokeToJson(stroke));
    }

    return json{
        {"schema", "perapera.layer.v1"},
        {"layerId", paddedName("layer_", layerIndex)},
        {"name", layer.name},
        {"visible", layer.visible},
        {"opacity", layer.opacity},
        {"blendMode", "normal"},
        {"strokes", strokes},
    };
}

bool saveFrame(
    const fs::path& frameDir,
    const AnimationFrame& frame,
    int frameIndex,
    DrawingNewLayoutSaveResult& result,
    std::string* errorMessage)
{
    const fs::path layersDir = frameDir / "layers";
    json frameJson{
        {"schema", "perapera.frame.v1"},
        {"name", frame.name},
        {"durationFrames", frame.durationFrames},
        {"layerCount", frame.layers.size()},
        {"layersDirectory", "layers"},
    };

    if (!writeJsonFile(frameDir / "frame.json", frameJson, errorMessage)) {
        return false;
    }

    for (std::size_t index = 0; index < frame.layers.size(); ++index) {
        const fs::path layerPath = layersDir / (paddedName("layer_", static_cast<int>(index)) + ".json");
        if (!writeJsonFile(layerPath, layerToJson(frame.layers[index], static_cast<int>(index)), errorMessage)) {
            return false;
        }
        ++result.layerCount;
        result.strokeCount += frame.layers[index].strokes.size();
    }

    (void)frameIndex;
    ++result.frameCount;
    return true;
}

bool saveCell(
    const fs::path& projectRoot,
    const std::string& sceneId,
    const std::string& cutId,
    const DrawingCell& cell,
    DrawingNewLayoutSaveResult& result,
    std::string* errorMessage)
{
    const std::string cellId = normalizeLayoutId(cell.id, "cell_001");
    const fs::path cellDir = cellDirectory(projectRoot, sceneId, cutId, cellId);
    json frames = json::array();

    for (std::size_t index = 0; index < cell.frames.size(); ++index) {
        const std::string frameName = frameDirectoryName(static_cast<int>(index));
        frames.push_back({
            {"name", cell.frames[index].name},
            {"durationFrames", cell.frames[index].durationFrames},
            {"path", std::string("frames/") + frameName + "/frame.json"},
        });
    }

    json cellJson{
        {"schema", "perapera.cell.v1"},
        {"id", cellId},
        {"name", cell.name},
        {"category", categoryToText(cell.category)},
        {"widthPx", cell.widthPx},
        {"heightPx", cell.heightPx},
        {"zOrder", cell.zOrder},
        {"visible", cell.visible},
        {"opacity", cell.opacity},
        {"placement", json{{"x", cell.positionX}, {"y", cell.positionY}, {"scale", cell.scale}, {"rotation", cell.rotationDegrees}}},
        {"frames", frames},
    };

    if (!writeJsonFile(cellDir / "cell.json", cellJson, errorMessage)) {
        return false;
    }

    for (std::size_t index = 0; index < cell.frames.size(); ++index) {
        if (!saveFrame(
                frameDirectory(projectRoot, sceneId, cutId, cellId, static_cast<int>(index)),
                cell.frames[index],
                static_cast<int>(index),
                result,
                errorMessage)) {
            return false;
        }
    }

    ++result.cellCount;
    return true;
}

DrawingLayer loadLayer(const fs::path& layerPath, std::size_t& strokeCount, std::string* errorMessage)
{
    json layerJson;
    DrawingLayer layer;
    if (!readJsonFile(layerPath, layerJson, errorMessage)) {
        return layer;
    }

    layer.name = layerJson.value("name", layer.name);
    layer.visible = layerJson.value("visible", layer.visible);
    layer.opacity = layerJson.value("opacity", layer.opacity);
    const json strokes = layerJson.value("strokes", json::array());
    if (strokes.is_array()) {
        for (const json& strokeJson : strokes) {
            layer.strokes.push_back(strokeFromJson(strokeJson));
            ++strokeCount;
        }
    }
    return layer;
}

bool loadFrame(
    const fs::path& cellDir,
    const json& frameEntry,
    bool requireLayerJson,
    AnimationFrame& frame,
    std::size_t& layerCount,
    std::size_t& strokeCount,
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

    frame.name = frameJson.value("name", frameEntry.value("name", frame.name));
    frame.durationFrames = frameJson.value("durationFrames", frameEntry.value("durationFrames", frame.durationFrames));
    frame.layers.clear();

    const std::size_t expectedLayerCount = frameJson.value("layerCount", 0U);
    const std::string layersDirectoryName = frameJson.value("layersDirectory", "layers");
    const fs::path layersDir = frameJsonPath.parent_path() / layersDirectoryName;
    const std::vector<fs::path> layerFiles = sortedJsonFiles(layersDir);
    if (requireLayerJson && layerFiles.size() < expectedLayerCount) {
        setError(errorMessage, "not enough layer json files in: " + layersDir.string());
        return false;
    }

    for (const fs::path& layerPath : layerFiles) {
        frame.layers.push_back(loadLayer(layerPath, strokeCount, errorMessage));
        ++layerCount;
    }
    return true;
}

bool loadCell(
    const fs::path& cellDir,
    bool requireLayerJson,
    DrawingCell& cell,
    DrawingNewLayoutLoadResult& result,
    std::string* errorMessage)
{
    const fs::path cellPath = cellDir / "cell.json";
    json cellJson;
    if (!readJsonFile(cellPath, cellJson, errorMessage)) {
        return false;
    }

    cell.id = cellJson.value("id", cellDir.filename().string());
    cell.name = cellJson.value("name", cell.name);
    cell.category = categoryFromText(cellJson.value("category", "other"));
    cell.widthPx = cellJson.value("widthPx", cell.widthPx);
    cell.heightPx = cellJson.value("heightPx", cell.heightPx);
    cell.zOrder = cellJson.value("zOrder", cell.zOrder);
    cell.visible = cellJson.value("visible", cell.visible);
    cell.opacity = cellJson.value("opacity", cell.opacity);

    const json placement = cellJson.value("placement", json::object());
    if (placement.is_object()) {
        cell.positionX = placement.value("x", cell.positionX);
        cell.positionY = placement.value("y", cell.positionY);
        cell.scale = placement.value("scale", cell.scale);
        cell.rotationDegrees = placement.value("rotation", cell.rotationDegrees);
    }

    const json frames = cellJson.value("frames", json::array());
    if (!frames.is_array()) {
        setError(errorMessage, "cell frames is not an array: " + cellPath.string());
        return false;
    }

    cell.frames.clear();
    for (const json& frameEntry : frames) {
        if (!frameEntry.is_object()) {
            setError(errorMessage, "frame entry is not an object: " + cellPath.string());
            return false;
        }
        AnimationFrame frame;
        if (!loadFrame(cellDir, frameEntry, requireLayerJson, frame, result.layerCount, result.strokeCount, errorMessage)) {
            return false;
        }
        cell.frames.push_back(std::move(frame));
        ++result.frameCount;
    }
    return true;
}

} // namespace

bool saveDrawingCellsToNewLayout(
    const std::filesystem::path& projectRoot,
    const DrawingNewLayoutIOOptions& options,
    const std::vector<DrawingCell>& cells,
    DrawingNewLayoutSaveResult* outResult,
    std::string* errorMessage)
{
    if (projectRoot.empty()) {
        setError(errorMessage, "projectRoot is empty");
        return false;
    }

    const std::string sceneId = normalizeLayoutId(options.sceneId, defaultSceneId());
    const std::string cutId = normalizeLayoutId(options.cutId, defaultCutId());
    const fs::path cellsDir = cellsDirectory(projectRoot, sceneId, cutId);

    std::error_code removeError;
    fs::remove_all(cellsDir, removeError);
    if (removeError) {
        setError(errorMessage, "failed to clear cells directory: " + cellsDir.string());
        return false;
    }

    DrawingNewLayoutSaveResult result;
    result.cellsDirectory = cellsDir;
    for (const DrawingCell& cell : cells) {
        if (!saveCell(projectRoot, sceneId, cutId, cell, result, errorMessage)) {
            return false;
        }
    }

    if (outResult != nullptr) {
        *outResult = result;
    }
    return true;
}

bool loadDrawingCellsFromNewLayout(
    const std::filesystem::path& projectRoot,
    const DrawingNewLayoutIOOptions& options,
    DrawingNewLayoutLoadResult* outResult,
    std::string* errorMessage)
{
    if (outResult == nullptr) {
        setError(errorMessage, "outResult is null");
        return false;
    }
    if (projectRoot.empty()) {
        setError(errorMessage, "projectRoot is empty");
        return false;
    }

    const std::string sceneId = normalizeLayoutId(options.sceneId, defaultSceneId());
    const std::string cutId = normalizeLayoutId(options.cutId, defaultCutId());
    const fs::path cellsDir = cellsDirectory(projectRoot, sceneId, cutId);
    if (!fs::is_directory(cellsDir)) {
        setError(errorMessage, "new layout cells directory not found: " + cellsDir.string());
        return false;
    }

    DrawingNewLayoutLoadResult result;
    result.cellsDirectory = cellsDir;
    for (const fs::path& cellDir : sortedDirectories(cellsDir)) {
        DrawingCell cell;
        if (!loadCell(cellDir, options.requireLayerJson, cell, result, errorMessage)) {
            return false;
        }
        result.cells.push_back(std::move(cell));
    }

    *outResult = std::move(result);
    return true;
}

} // namespace perapera
