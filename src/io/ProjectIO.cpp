// このファイルの役割:
// Project を project.json + cells/ 以下のJSON群として保存・読み込みする。
// UIや描画キャッシュには依存せず、ドメインデータだけを扱う。

#include "io/ProjectIO.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

#include <nlohmann/json.hpp>

namespace perapera {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

std::string numberedName(const char* prefix, int number)
{
    std::ostringstream stream;
    stream << prefix << '_' << std::setw(3) << std::setfill('0') << number;
    return stream.str();
}
bool writeJsonFile(const fs::path& path, const json& value, std::string* errorMessage)
{
    std::ofstream file(path);
    if (!file) {
        setError(errorMessage, "failed to open for write: " + path.string());
        return false;
    }
    file << std::setw(4) << value << '\n';
    if (!file) {
        setError(errorMessage, "failed to write json: " + path.string());
        return false;
    }
    return true;
}
bool readJsonFile(const fs::path& path, json& value, std::string* errorMessage)
{
    std::ifstream file(path);
    if (!file) {
        setError(errorMessage, "failed to open for read: " + path.string());
        return false;
    }
    try {
        file >> value;
    } catch (const std::exception& exception) {
        setError(errorMessage, "failed to parse json: " + path.string() + " / " + exception.what());
        return false;
    }
    return true;
}
json toJson(const StrokePoint& point)
{
    return json{{"x", point.x}, {"y", point.y}, {"pressure", point.pressure}};
}
StrokePoint strokePointFromJson(const json& value)
{
    StrokePoint point;
    point.x = value.value("x", 0.0f);
    point.y = value.value("y", 0.0f);
    point.pressure = value.value("pressure", 1.0f);
    return point;
}
json toJson(const Stroke& stroke)
{
    json points = json::array();
    for (const StrokePoint& point : stroke.points) {
        points.push_back(toJson(point));
    }

    return json{{"color", stroke.color}, {"radiusPx", stroke.radiusPx}, {"points", points}};
}
Stroke strokeFromJson(const json& value)
{
    Stroke stroke;
    if (value.contains("color") && value.at("color").is_array() && value.at("color").size() == 4) {
        for (std::size_t index = 0; index < 4; ++index) {
            stroke.color[index] = value.at("color").at(index).get<float>();
        }
    }
    stroke.radiusPx = value.value("radiusPx", 4.0f);

    if (value.contains("points") && value.at("points").is_array()) {
        for (const json& pointJson : value.at("points")) {
            stroke.points.push_back(strokePointFromJson(pointJson));
        }
    }
    return stroke;
}
json toJson(const Layer& layer)
{
    json strokes = json::array();
    for (const Stroke& stroke : layer.strokes) {
        strokes.push_back(toJson(stroke));
    }

    return json{
        {"layerId", layer.layerId},
        {"name", layer.name},
        {"visible", layer.visible},
        {"opacity", layer.opacity},
        {"blendMode", layer.blendMode},
        {"strokes", strokes},
    };
}

Layer layerFromJson(const json& value)
{
    Layer layer;
    layer.layerId = value.value("layerId", std::string("layer_001"));
    layer.name = value.value("name", std::string("線画"));
    layer.visible = value.value("visible", true);
    layer.opacity = value.value("opacity", 1.0f);
    layer.blendMode = value.value("blendMode", std::string("normal"));

    if (value.contains("strokes") && value.at("strokes").is_array()) {
        for (const json& strokeJson : value.at("strokes")) {
            layer.strokes.push_back(strokeFromJson(strokeJson));
        }
    }
    return layer;
}
json frameMetaToJson(const Frame& frame)
{
    return json{{"name", frame.name}, {"durationFrames", frame.durationFrames}};
}
Frame frameMetaFromJson(const json& value, int frameNumber)
{
    Frame frame = Frame::createDefault(frameNumber);
    frame.name = value.value("name", frame.name);
    frame.durationFrames = value.value("durationFrames", frame.durationFrames);
    frame.layers.clear();
    return frame;
}
json toJson(const CellPlacement& placement)
{
    return json{
        {"x", placement.x},
        {"y", placement.y},
        {"scale", placement.scale},
        {"rotation", placement.rotation},
    };
}

CellPlacement placementFromJson(const json& value)
{
    CellPlacement placement;
    placement.x = value.value("x", 0.0f);
    placement.y = value.value("y", 0.0f);
    placement.scale = value.value("scale", 1.0f);
    placement.rotation = value.value("rotation", 0.0f);
    return placement;
}
json toJson(const CellMotionKey& key)
{
    return json{
        {"frame", key.frame},
        {"placement", toJson(key.placement)},
        {"interpolation", key.interpolation},
    };
}

CellMotionKey motionKeyFromJson(const json& value)
{
    CellMotionKey key;
    key.frame = value.value("frame", 0);
    if (value.contains("placement") && value.at("placement").is_object()) {
        key.placement = placementFromJson(value.at("placement"));
    }
    key.interpolation = value.value("interpolation", std::string("linear"));
    return key;
}
json cellMetaToJson(const Cell& cell)
{
    json motionKeys = json::array();
    for (const CellMotionKey& key : cell.motionKeys) {
        motionKeys.push_back(toJson(key));
    }

    return json{
        {"id", cell.id},
        {"name", cell.name},
        {"category", cell.category},
        {"width", cell.widthPx},
        {"height", cell.heightPx},
        {"zOrder", cell.zOrder},
        {"visible", cell.visible},
        {"opacity", cell.opacity},
        {"placement", toJson(cell.placement)},
        {"motionKeys", motionKeys},
    };
}

Cell cellMetaFromJson(const json& value)
{
    Cell cell = Cell::createDefault();
    cell.id = value.value("id", cell.id);
    cell.name = value.value("name", cell.name);
    cell.category = value.value("category", cell.category);
    cell.widthPx = value.value("width", cell.widthPx);
    cell.heightPx = value.value("height", cell.heightPx);
    cell.zOrder = value.value("zOrder", cell.zOrder);
    cell.visible = value.value("visible", cell.visible);
    cell.opacity = value.value("opacity", cell.opacity);
    cell.frames.clear();

    if (value.contains("placement") && value.at("placement").is_object()) {
        cell.placement = placementFromJson(value.at("placement"));
    }
    if (value.contains("motionKeys") && value.at("motionKeys").is_array()) {
        for (const json& keyJson : value.at("motionKeys")) {
            cell.motionKeys.push_back(motionKeyFromJson(keyJson));
        }
    }
    return cell;
}
json projectToJson(const Project& project)
{
    json cameraKeys = json::array();
    for (const CameraKey& key : project.camera.keys) {
        cameraKeys.push_back(json{
            {"frame", key.frame},
            {"centerX", key.centerX},
            {"centerY", key.centerY},
            {"zoom", key.zoom},
        });
    }

    return json{
        {"version", project.version},
        {"name", project.name},
        {"canvas", {{"width", project.canvas.width}, {"height", project.canvas.height}}},
        {"output", {
            {"width", project.output.width},
            {"height", project.output.height},
            {"fps", project.output.fps},
            {"pixelAspect", project.output.pixelAspect},
        }},
        {"timeline", {{"totalFrames", project.timeline.totalFrames}}},
        {"camera", {
            {"centerX", project.camera.centerX},
            {"centerY", project.camera.centerY},
            {"zoom", project.camera.zoom},
            {"animationEnabled", project.camera.animationEnabled},
            {"keys", cameraKeys},
        }},
        {"audio", {
            {"enabled", project.audio.enabled},
            {"filePath", project.audio.filePath},
            {"startFrame", project.audio.startFrame},
        }},
        {"cellOrder", project.cellOrder},
    };
}

Project projectFromJson(const json& value)
{
    Project project = Project::createDefault();
    project.version = value.value("version", project.version);
    project.name = value.value("name", project.name);
    project.cells.clear();
    project.cellOrder.clear();

    if (value.contains("canvas") && value.at("canvas").is_object()) {
        const json& canvas = value.at("canvas");
        project.canvas.width = canvas.value("width", project.canvas.width);
        project.canvas.height = canvas.value("height", project.canvas.height);
    }
    if (value.contains("output") && value.at("output").is_object()) {
        const json& output = value.at("output");
        project.output.width = output.value("width", project.output.width);
        project.output.height = output.value("height", project.output.height);
        project.output.fps = output.value("fps", project.output.fps);
        project.output.pixelAspect = output.value("pixelAspect", project.output.pixelAspect);
    }
    if (value.contains("timeline") && value.at("timeline").is_object()) {
        project.timeline.totalFrames = value.at("timeline").value("totalFrames", project.timeline.totalFrames);
    }
    if (value.contains("camera") && value.at("camera").is_object()) {
        const json& camera = value.at("camera");
        project.camera.centerX = camera.value("centerX", project.camera.centerX);
        project.camera.centerY = camera.value("centerY", project.camera.centerY);
        project.camera.zoom = camera.value("zoom", project.camera.zoom);
        project.camera.animationEnabled = camera.value("animationEnabled", project.camera.animationEnabled);
    }
    if (value.contains("audio") && value.at("audio").is_object()) {
        const json& audio = value.at("audio");
        project.audio.enabled = audio.value("enabled", project.audio.enabled);
        project.audio.filePath = audio.value("filePath", project.audio.filePath);
        project.audio.startFrame = audio.value("startFrame", project.audio.startFrame);
    }
    if (value.contains("cellOrder") && value.at("cellOrder").is_array()) {
        project.cellOrder = value.at("cellOrder").get<std::vector<std::string>>();
    }
    return project;
}
bool saveCell(const Cell& cell, const fs::path& cellsRoot, std::string* errorMessage)
{
    const fs::path cellFolder = cellsRoot / cell.id;
    const fs::path framesRoot = cellFolder / "frames";
    std::error_code errorCode;
    fs::create_directories(framesRoot, errorCode);
    if (errorCode) {
        setError(errorMessage, "failed to create cell folders: " + cellFolder.string());
        return false;
    }
    if (!writeJsonFile(cellFolder / "cell.json", cellMetaToJson(cell), errorMessage)) {
        return false;
    }
    for (std::size_t frameIndex = 0; frameIndex < cell.frames.size(); ++frameIndex) {
        const Frame& frame = cell.frames[frameIndex];
        const fs::path frameFolder = framesRoot / numberedName("frame", static_cast<int>(frameIndex + 1));
        fs::create_directories(frameFolder, errorCode);
        if (errorCode) {
            setError(errorMessage, "failed to create frame folder: " + frameFolder.string());
            return false;
        }
        if (!writeJsonFile(frameFolder / "frame.json", frameMetaToJson(frame), errorMessage)) {
            return false;
        }
        for (std::size_t layerIndex = 0; layerIndex < frame.layers.size(); ++layerIndex) {
            const fs::path layerPath = frameFolder / (numberedName("layer", static_cast<int>(layerIndex + 1)) + ".json");
            if (!writeJsonFile(layerPath, toJson(frame.layers[layerIndex]), errorMessage)) {
                return false;
            }
        }
    }
    return true;
}
std::vector<fs::path> sortedDirectories(const fs::path& folder)
{
    std::vector<fs::path> result;
    std::error_code errorCode;
    if (!fs::exists(folder, errorCode)) {
        return result;
    }
    for (const fs::directory_entry& entry : fs::directory_iterator(folder, errorCode)) {
        if (entry.is_directory()) {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}
std::vector<fs::path> sortedLayerFiles(const fs::path& folder)
{
    std::vector<fs::path> result;
    std::error_code errorCode;
    for (const fs::directory_entry& entry : fs::directory_iterator(folder, errorCode)) {
        if (entry.is_regular_file() && entry.path().filename().string().rfind("layer_", 0) == 0) {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}
bool loadCellFrames(Cell& cell, const fs::path& cellFolder, std::string* errorMessage)
{
    const fs::path framesRoot = cellFolder / "frames";
    const std::vector<fs::path> frameFolders = sortedDirectories(framesRoot);
    int frameNumber = 1;

    for (const fs::path& frameFolder : frameFolders) {
        json frameJson;
        if (!readJsonFile(frameFolder / "frame.json", frameJson, errorMessage)) {
            return false;
        }
        Frame frame = frameMetaFromJson(frameJson, frameNumber);
        const std::vector<fs::path> layerFiles = sortedLayerFiles(frameFolder);
        for (const fs::path& layerFile : layerFiles) {
            json layerJson;
            if (!readJsonFile(layerFile, layerJson, errorMessage)) {
                return false;
            }
            frame.layers.push_back(layerFromJson(layerJson));
        }
        if (frame.layers.empty()) {
            frame.layers.push_back(Layer::createDefault(1));
        }
        cell.frames.push_back(frame);
        ++frameNumber;
    }

    if (cell.frames.empty()) {
        cell.frames.push_back(Frame::createDefault(1));
    }
    return true;
}
std::vector<std::string> discoverCellIds(const fs::path& cellsRoot)
{
    std::vector<std::string> result;
    for (const fs::path& cellFolder : sortedDirectories(cellsRoot)) {
        result.push_back(cellFolder.filename().string());
    }
    return result;
}
} // namespace

bool ProjectIO::save(const Project& project, const fs::path& folderPath, std::string* errorMessage)
{
    std::error_code errorCode;
    fs::create_directories(folderPath, errorCode);
    if (errorCode) {
        setError(errorMessage, "failed to create project folder: " + folderPath.string());
        return false;
    }
    if (!writeJsonFile(folderPath / "project.json", projectToJson(project), errorMessage)) {
        return false;
    }
    const fs::path cellsRoot = folderPath / "cells";
    fs::remove_all(cellsRoot, errorCode);
    errorCode.clear();
    fs::create_directories(cellsRoot, errorCode);
    if (errorCode) {
        setError(errorMessage, "failed to create cells folder: " + cellsRoot.string());
        return false;
    }
    for (const Cell& cell : project.cells) {
        if (!saveCell(cell, cellsRoot, errorMessage)) {
            return false;
        }
    }
    return true;
}
bool ProjectIO::load(const fs::path& folderPath, Project& project, std::string* errorMessage)
{
    project = Project::createDefault();

    json projectJson;
    if (!readJsonFile(folderPath / "project.json", projectJson, errorMessage)) {
        return false;
    }
    Project loadedProject = projectFromJson(projectJson);
    const fs::path cellsRoot = folderPath / "cells";
    std::vector<std::string> cellIds = loadedProject.cellOrder;
    if (cellIds.empty()) {
        cellIds = discoverCellIds(cellsRoot);
    }

    for (const std::string& cellId : cellIds) {
        const fs::path cellFolder = cellsRoot / cellId;
        json cellJson;
        if (!readJsonFile(cellFolder / "cell.json", cellJson, errorMessage)) {
            return false;
        }
        Cell cell = cellMetaFromJson(cellJson);
        if (!loadCellFrames(cell, cellFolder, errorMessage)) {
            return false;
        }
        loadedProject.cells.push_back(cell);
    }

    if (loadedProject.cells.empty()) {
        Cell defaultCell = Cell::createDefault();
        loadedProject.cellOrder = {defaultCell.id};
        loadedProject.cells.push_back(defaultCell);
    }

    project = loadedProject;
    return true;
}
} // namespace perapera
