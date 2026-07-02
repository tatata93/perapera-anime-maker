// このファイルの役割:
// Project を project.json + cells/ 以下のJSON群として保存・読み込みする。
// UIや描画キャッシュには依存せず、ドメインデータだけを扱う。
// Phase 1.5 Step 19では、バケツ塗り専用FillStrokeの1chマスクを保存する。
// Phase 1.5 Step 19k: FillStrokeのbitmapはJSONへBase64埋め込みせず、layer_NNN_fills.binへ分離して保存する。
// Phase 1.5 Step 19l: fills.binを width*height 生バイト形式へ整理し、Base64保存経路を使わない。
// Phase 1.5 Step 21: FillStrokeをbitmapX/Y付きのクロップマスクとして保存する。

#include "io/ProjectIO.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <set>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace perapera {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

constexpr int kProjectFormatVersion = 1;
constexpr std::uint32_t kFillBinMagicV2 = 0x32464650U; // "PFF2" little-endian marker.

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

void appendUint32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    const auto* raw = reinterpret_cast<const std::uint8_t*>(&value);
    bytes.insert(bytes.end(), raw, raw + sizeof(value));
}

void appendFloat32(std::vector<std::uint8_t>& bytes, float value)
{
    const auto* raw = reinterpret_cast<const std::uint8_t*>(&value);
    bytes.insert(bytes.end(), raw, raw + sizeof(value));
}

bool readUint32(std::ifstream& file, std::uint32_t& value)
{
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return static_cast<bool>(file);
}

bool readFloat32(std::ifstream& file, float& value)
{
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return static_cast<bool>(file);
}

bool writeJsonFile(const fs::path& path, const json& value, std::string* errorMessage)
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

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(errorMessage, "failed to open for write: " + path.string());
        return false;
    }
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!file) {
        setError(errorMessage, "failed to write json: " + path.string());
        return false;
    }
    return true;
}

bool writeBinaryFileIfChanged(const fs::path& path, const std::vector<std::uint8_t>& bytes, std::string* errorMessage)
{
    std::error_code errorCode;
    const auto existingSize = fs::file_size(path, errorCode);
    if (!errorCode && existingSize == bytes.size()) {
        std::ifstream existing(path, std::ios::binary);
        if (existing) {
            std::vector<std::uint8_t> existingBytes(bytes.size());
            existing.read(reinterpret_cast<char*>(existingBytes.data()), static_cast<std::streamsize>(existingBytes.size()));
            if (existing && existingBytes == bytes) {
                return true;
            }
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(errorMessage, "failed to open binary for write: " + path.string());
        return false;
    }
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        setError(errorMessage, "failed to write binary: " + path.string());
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

std::vector<fs::path> sortedDirectories(const fs::path& root)
{
    std::vector<fs::path> result;
    std::error_code errorCode;
    if (!fs::exists(root, errorCode)) {
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

std::vector<fs::path> sortedLayerFiles(const fs::path& frameFolder)
{
    std::vector<fs::path> result;
    std::error_code errorCode;
    if (!fs::exists(frameFolder, errorCode)) {
        return result;
    }
    for (const fs::directory_entry& entry : fs::directory_iterator(frameFolder, errorCode)) {
        if (errorCode || !entry.is_regular_file()) {
            continue;
        }
        const std::string filename = entry.path().filename().string();
        if (filename.rfind("layer_", 0) == 0 && entry.path().extension() == ".json") {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool containsName(const std::set<std::string>& names, const std::string& name)
{
    return names.find(name) != names.end();
}

bool fillStrokeBitmapSizeIsValid(const Stroke& stroke)
{
    if (stroke.brushEngine != StrokeBrushEngine::Fill || stroke.bitmapWidth <= 0 || stroke.bitmapHeight <= 0) {
        return false;
    }
    const std::uint64_t expectedSize = static_cast<std::uint64_t>(stroke.bitmapWidth) *
                                       static_cast<std::uint64_t>(stroke.bitmapHeight);
    return expectedSize == static_cast<std::uint64_t>(stroke.bitmap.size());
}

bool fillStrokeHasPaint(const Stroke& stroke)
{
    return fillStrokeBitmapSizeIsValid(stroke) &&
        std::any_of(stroke.bitmap.begin(), stroke.bitmap.end(), [](std::uint8_t value) {
            return value != 0U;
        });
}

bool cropFillStrokeToContent(Stroke& stroke)
{
    if (!fillStrokeBitmapSizeIsValid(stroke)) {
        return false;
    }

    int minX = stroke.bitmapWidth;
    int minY = stroke.bitmapHeight;
    int maxX = -1;
    int maxY = -1;
    for (int y = 0; y < stroke.bitmapHeight; ++y) {
        for (int x = 0; x < stroke.bitmapWidth; ++x) {
            const std::uint8_t mask = stroke.bitmap[static_cast<std::size_t>(y) *
                                                    static_cast<std::size_t>(stroke.bitmapWidth) +
                                                    static_cast<std::size_t>(x)];
            if (mask == 0U) {
                continue;
            }
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        }
    }

    if (maxX < minX || maxY < minY) {
        stroke.bitmap.clear();
        stroke.bitmapWidth = 0;
        stroke.bitmapHeight = 0;
        return false;
    }

    if (minX == 0 && minY == 0 && maxX == stroke.bitmapWidth - 1 && maxY == stroke.bitmapHeight - 1) {
        return true;
    }

    const int croppedWidth = maxX - minX + 1;
    const int croppedHeight = maxY - minY + 1;
    std::vector<std::uint8_t> cropped(static_cast<std::size_t>(croppedWidth) *
                                      static_cast<std::size_t>(croppedHeight), 0U);
    for (int y = 0; y < croppedHeight; ++y) {
        for (int x = 0; x < croppedWidth; ++x) {
            cropped[static_cast<std::size_t>(y) * static_cast<std::size_t>(croppedWidth) +
                    static_cast<std::size_t>(x)] =
                stroke.bitmap[static_cast<std::size_t>(minY + y) *
                              static_cast<std::size_t>(stroke.bitmapWidth) +
                              static_cast<std::size_t>(minX + x)];
        }
    }

    stroke.bitmapX += minX;
    stroke.bitmapY += minY;
    stroke.bitmapWidth = croppedWidth;
    stroke.bitmapHeight = croppedHeight;
    stroke.bitmap = std::move(cropped);
    return true;
}

void removeStaleDirectories(const fs::path& root, const std::set<std::string>& expectedNames)
{
    std::error_code errorCode;
    if (!fs::exists(root, errorCode)) {
        return;
    }
    for (const fs::directory_entry& entry : fs::directory_iterator(root, errorCode)) {
        if (errorCode || !entry.is_directory()) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        if (!containsName(expectedNames, name)) {
            fs::remove_all(entry.path(), errorCode);
            errorCode.clear();
        }
    }
}

void removeStaleLayerFiles(const fs::path& frameFolder, const std::set<std::string>& expectedLayerStems)
{
    std::error_code errorCode;
    if (!fs::exists(frameFolder, errorCode)) {
        return;
    }
    for (const fs::directory_entry& entry : fs::directory_iterator(frameFolder, errorCode)) {
        if (errorCode || !entry.is_regular_file()) {
            continue;
        }

        const fs::path path = entry.path();
        const std::string filename = path.filename().string();
        if (filename.rfind("layer_", 0) != 0) {
            continue;
        }

        std::string stem;
        if (path.extension() == ".json") {
            stem = path.stem().string();
        } else if (filename.size() > std::string{"_fills.bin"}.size()
                   && filename.rfind("_fills.bin") == filename.size() - std::string{"_fills.bin"}.size()) {
            stem = filename.substr(0, filename.size() - std::string{"_fills.bin"}.size());
        } else {
            continue;
        }

        if (!containsName(expectedLayerStems, stem)) {
            fs::remove(path, errorCode);
            errorCode.clear();
        }
    }
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

json toJson(const Stroke& stroke, int fillIndex = -1)
{
    if (stroke.brushEngine == StrokeBrushEngine::Fill) {
        return json{
            {"brushEngine", "Fill"},
            {"fillIndex", fillIndex},
            {"bitmapX", stroke.bitmapX},
            {"bitmapY", stroke.bitmapY},
            {"bitmapWidth", stroke.bitmapWidth},
            {"bitmapHeight", stroke.bitmapHeight},
        };
    }

    json points = json::array();
    for (const StrokePoint& point : stroke.points) {
        points.push_back(toJson(point));
    }

    return json{
        {"color", stroke.color},
        {"radiusPx", stroke.radiusPx},
        {"brushEngine", strokeBrushEngineToString(stroke.brushEngine)},
        {"opacity", stroke.opacity},
        {"hardness", stroke.hardness},
        {"spacing", stroke.spacing},
        {"pressureToSize", stroke.pressureToSize},
        {"pressureToOpacity", stroke.pressureToOpacity},
        {"watercolorBleed", stroke.watercolorBleed},
        {"colorMix", stroke.colorMix},
        {"dryRate", stroke.dryRate},
        {"points", points},
    };
}

std::vector<Stroke> loadFillStrokesFile(const fs::path& path)
{
    std::vector<Stroke> result;
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return result;
    }

    std::uint32_t firstValue = 0U;
    if (!readUint32(file, firstValue)) {
        return result;
    }
    const bool hasV2Header = firstValue == kFillBinMagicV2;
    std::uint32_t strokeCount = firstValue;
    if (hasV2Header && !readUint32(file, strokeCount)) {
        return result;
    }

    result.reserve(strokeCount);
    for (std::uint32_t strokeIndex = 0U; strokeIndex < strokeCount; ++strokeIndex) {
        std::uint32_t bitmapX = 0U;
        std::uint32_t bitmapY = 0U;
        std::uint32_t bitmapWidth = 0U;
        std::uint32_t bitmapHeight = 0U;
        Stroke stroke;
        stroke.brushEngine = StrokeBrushEngine::Fill;

        if (hasV2Header && (!readUint32(file, bitmapX) || !readUint32(file, bitmapY))) {
            return {};
        }
        if (!readUint32(file, bitmapWidth) || !readUint32(file, bitmapHeight)) {
            return {};
        }
        for (float& component : stroke.color) {
            if (!readFloat32(file, component)) {
                return {};
            }
        }
        if (!readFloat32(file, stroke.opacity)) {
            return {};
        }

        const std::uint64_t expectedSize = static_cast<std::uint64_t>(bitmapWidth) * static_cast<std::uint64_t>(bitmapHeight);
        if (bitmapWidth == 0U || bitmapHeight == 0U || expectedSize > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
            return {};
        }

        stroke.bitmapX = static_cast<int>(bitmapX);
        stroke.bitmapY = static_cast<int>(bitmapY);
        stroke.bitmapWidth = static_cast<int>(bitmapWidth);
        stroke.bitmapHeight = static_cast<int>(bitmapHeight);
        stroke.opacity = std::clamp(stroke.opacity, 0.05f, 1.0f);
        stroke.bitmap.resize(static_cast<std::size_t>(expectedSize));
        file.read(reinterpret_cast<char*>(stroke.bitmap.data()), static_cast<std::streamsize>(stroke.bitmap.size()));
        if (!file) {
            return {};
        }
        if (!cropFillStrokeToContent(stroke)) {
            continue;
        }
        result.push_back(std::move(stroke));
    }

    return result;
}

bool saveFillStrokesFile(const fs::path& path, const Layer& layer, std::string* errorMessage)
{
    std::uint32_t fillCount = 0U;
    for (const Stroke& stroke : layer.strokes) {
        if (fillStrokeHasPaint(stroke)) {
            ++fillCount;
        }
    }

    if (fillCount == 0U) {
        std::error_code removeError;
        fs::remove(path, removeError);
        return true;
    }

    std::vector<std::uint8_t> bytes;
    bytes.reserve(8U);
    appendUint32(bytes, kFillBinMagicV2);
    appendUint32(bytes, fillCount);
    for (const Stroke& stroke : layer.strokes) {
        if (!fillStrokeHasPaint(stroke)) {
            continue;
        }
        Stroke storedStroke = stroke;
        if (!cropFillStrokeToContent(storedStroke)) {
            continue;
        }

        appendUint32(bytes, static_cast<std::uint32_t>(storedStroke.bitmapX));
        appendUint32(bytes, static_cast<std::uint32_t>(storedStroke.bitmapY));
        appendUint32(bytes, static_cast<std::uint32_t>(storedStroke.bitmapWidth));
        appendUint32(bytes, static_cast<std::uint32_t>(storedStroke.bitmapHeight));
        for (float component : storedStroke.color) {
            appendFloat32(bytes, component);
        }
        appendFloat32(bytes, storedStroke.opacity);
        bytes.insert(bytes.end(), storedStroke.bitmap.begin(), storedStroke.bitmap.end());
    }

    return writeBinaryFileIfChanged(path, bytes, errorMessage);
}

std::optional<Stroke> strokeFromJson(const json& value, const std::vector<Stroke>& fillStrokes)
{
    const std::string engineText = value.value("brushEngine", std::string("Simple"));
    const bool isSimple = engineText == "Simple" || engineText == "SimpleBrushEngine" || engineText.empty();
    const bool isMyPaint = engineText == "MyPaint" || engineText == "MyPaintBrushEngine";
    const bool isFill = engineText == "Fill" || engineText == "FillStroke";
    if (!isSimple && !isMyPaint && !isFill) {
        return std::nullopt;
    }

    if (isFill) {
        const int fillIndex = value.value("fillIndex", -1);
        if (fillIndex < 0 || fillIndex >= static_cast<int>(fillStrokes.size())) {
            return std::nullopt;
        }
        return fillStrokes[static_cast<std::size_t>(fillIndex)];
    }

    Stroke stroke;
    if (value.contains("color") && value.at("color").is_array() && value.at("color").size() == 4) {
        for (std::size_t index = 0; index < 4; ++index) {
            stroke.color[index] = value.at("color").at(index).get<float>();
        }
    }
    stroke.opacity = std::clamp(value.value("opacity", 1.0f), 0.05f, 1.0f);
    stroke.radiusPx = value.value("radiusPx", 4.0f);
    stroke.brushEngine = isMyPaint ? StrokeBrushEngine::MyPaint : StrokeBrushEngine::Simple;
    stroke.hardness = std::clamp(value.value("hardness", 1.0f), 0.0f, 1.0f);
    stroke.spacing = std::clamp(value.value("spacing", 0.25f), 0.05f, 1.0f);
    stroke.pressureToSize = std::clamp(value.value("pressureToSize", 0.0f), 0.0f, 1.0f);
    stroke.pressureToOpacity = std::clamp(value.value("pressureToOpacity", 0.0f), 0.0f, 1.0f);
    stroke.watercolorBleed = std::clamp(value.value("watercolorBleed", 0.0f), 0.0f, 1.0f);
    stroke.colorMix = std::clamp(value.value("colorMix", 0.0f), 0.0f, 1.0f);
    stroke.dryRate = std::clamp(value.value("dryRate", 1.0f), 0.0f, 1.0f);

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
    int fillIndex = 0;
    for (const Stroke& stroke : layer.strokes) {
        if (stroke.brushEngine == StrokeBrushEngine::Fill) {
            if (!fillStrokeHasPaint(stroke)) {
                continue;
            }
            strokes.push_back(toJson(stroke, fillIndex));
            ++fillIndex;
        } else {
            strokes.push_back(toJson(stroke));
        }
    }

    return json{
        {"format_version", kProjectFormatVersion},
        {"layerId", layer.layerId},
        {"name", layer.name},
        {"visible", layer.visible},
        {"opacity", layer.opacity},
        {"blendMode", layer.blendMode},
        {"type", layerTypeToString(layer.type)},
        {"strokes", strokes},
    };
}

Layer layerFromJson(const json& value, const std::vector<Stroke>& fillStrokes)
{
    Layer layer;
    layer.layerId = value.value("layerId", std::string("layer_001"));
    layer.name = value.value("name", std::string("線画"));
    layer.visible = value.value("visible", true);
    layer.opacity = value.value("opacity", 1.0f);
    layer.blendMode = value.value("blendMode", std::string("normal"));
    layer.type = layerTypeFromString(value.value("type", std::string("Normal")));

    if (value.contains("strokes") && value.at("strokes").is_array()) {
        for (const json& strokeJson : value.at("strokes")) {
            std::optional<Stroke> stroke = strokeFromJson(strokeJson, fillStrokes);
            if (stroke.has_value()) {
                layer.strokes.push_back(std::move(*stroke));
            }
        }
    }
    return layer;
}

json frameMetaToJson(const Frame& frame)
{
    return json{{"format_version", kProjectFormatVersion}, {"name", frame.name}, {"durationFrames", frame.durationFrames}};
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
    return json{{"x", placement.x}, {"y", placement.y}, {"scale", placement.scale}, {"rotation", placement.rotation}};
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
    return json{{"frame", key.frame}, {"placement", toJson(key.placement)}, {"interpolation", key.interpolation}};
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
        {"format_version", kProjectFormatVersion},
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
        cameraKeys.push_back(json{{"frame", key.frame}, {"centerX", key.centerX}, {"centerY", key.centerY}, {"zoom", key.zoom}});
    }

    return json{
        {"format_version", kProjectFormatVersion},
        {"version", project.version},
        {"name", project.name},
        {"canvas", {{"width", project.canvas.width}, {"height", project.canvas.height}}},
        {"output", {{"width", project.output.width}, {"height", project.output.height}, {"fps", project.output.fps}, {"pixelAspect", project.output.pixelAspect}}},
        {"timeline", {{"totalFrames", project.timeline.totalFrames}}},
        {"camera", {{"centerX", project.camera.centerX}, {"centerY", project.camera.centerY}, {"zoom", project.camera.zoom}, {"animationEnabled", project.camera.animationEnabled}, {"keys", cameraKeys}}},
        {"audio", {{"enabled", project.audio.enabled}, {"filePath", project.audio.filePath}, {"startFrame", project.audio.startFrame}}},
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
        project.camera.keys.clear();
        if (camera.contains("keys") && camera.at("keys").is_array()) {
            for (const json& keyJson : camera.at("keys")) {
                CameraKey key;
                key.frame = keyJson.value("frame", key.frame);
                key.centerX = keyJson.value("centerX", key.centerX);
                key.centerY = keyJson.value("centerY", key.centerY);
                key.zoom = keyJson.value("zoom", key.zoom);
                project.camera.keys.push_back(key);
            }
        }
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

bool saveFrame(const Frame& frame, const fs::path& frameFolder, std::string* errorMessage)
{
    std::error_code errorCode;
    fs::create_directories(frameFolder, errorCode);
    if (errorCode) {
        setError(errorMessage, "failed to create frame folder: " + frameFolder.string());
        return false;
    }
    if (!writeJsonFile(frameFolder / "frame.json", frameMetaToJson(frame), errorMessage)) {
        return false;
    }
    std::set<std::string> expectedLayerStems;
    for (std::size_t layerIndex = 0; layerIndex < frame.layers.size(); ++layerIndex) {
        const std::string layerStem = numberedName("layer", static_cast<int>(layerIndex + 1U));
        expectedLayerStems.insert(layerStem);
        const fs::path layerPath = frameFolder / (layerStem + ".json");
        const fs::path fillsPath = frameFolder / (layerStem + "_fills.bin");
        if (!saveFillStrokesFile(fillsPath, frame.layers[layerIndex], errorMessage)) {
            return false;
        }
        if (!writeJsonFile(layerPath, toJson(frame.layers[layerIndex]), errorMessage)) {
            return false;
        }
    }
    removeStaleLayerFiles(frameFolder, expectedLayerStems);
    return true;
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
    std::set<std::string> expectedFrameFolders;
    for (std::size_t frameIndex = 0; frameIndex < cell.frames.size(); ++frameIndex) {
        const Frame& frame = cell.frames[frameIndex];
        const std::string frameFolderName = numberedName("frame", static_cast<int>(frameIndex + 1U));
        expectedFrameFolders.insert(frameFolderName);
        const fs::path frameFolder = framesRoot / frameFolderName;
        if (!saveFrame(frame, frameFolder, errorMessage)) {
            return false;
        }
    }
    removeStaleDirectories(framesRoot, expectedFrameFolders);
    return true;
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
            const fs::path fillsPath = layerFile.parent_path() / (layerFile.stem().string() + "_fills.bin");
            const std::vector<Stroke> fillStrokes = loadFillStrokesFile(fillsPath);
            frame.layers.push_back(layerFromJson(layerJson, fillStrokes));
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
    fs::create_directories(cellsRoot, errorCode);
    if (errorCode) {
        setError(errorMessage, "failed to create cells folder: " + cellsRoot.string());
        return false;
    }
    std::set<std::string> expectedCellFolders;
    for (const Cell& cell : project.cells) {
        expectedCellFolders.insert(cell.id);
        if (!saveCell(cell, cellsRoot, errorMessage)) {
            return false;
        }
    }
    removeStaleDirectories(cellsRoot, expectedCellFolders);
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
