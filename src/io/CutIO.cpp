// このファイルの役割:
// 正式タイムシートv1を持つCut単位の保存・読み込みを実装する。
// cut.json はカットメタ情報、timesheet.json はタイムシート本体として分離して扱う。

#include "io/CutIO.h"

#include "io/TimesheetIO.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

namespace perapera::CutIO {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

constexpr int kCutFormatVersion = 1;
constexpr const char* kCutKind = "perapera.cut.v1";

void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
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
            setError(errorMessage, "failed to create folder for cut: " + parent.string());
            return false;
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(errorMessage, "failed to open cut json for write: " + path.string());
        return false;
    }

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!file) {
        setError(errorMessage, "failed to write cut json: " + path.string());
        return false;
    }

    return true;
}

bool readJsonFile(const fs::path& path, json& value, std::string* errorMessage)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        setError(errorMessage, "failed to open cut json for read: " + path.string());
        return false;
    }

    try {
        file >> value;
    } catch (const std::exception& exception) {
        setError(errorMessage, "failed to parse cut json: " + path.string() + " / " + exception.what());
        return false;
    }

    return true;
}

int jsonInt(const json& value, const char* key, int fallback)
{
    return value.value(key, fallback);
}

float jsonFloat(const json& value, const char* key, float fallback)
{
    return value.value(key, fallback);
}

json cameraToJson(const CameraSettings& camera)
{
    json keys = json::array();
    for (const CameraKey& key : camera.keys) {
        keys.push_back({
            {"frame", key.frame},
            {"centerX", key.centerX},
            {"centerY", key.centerY},
            {"zoom", key.zoom},
        });
    }

    return json{
        {"centerX", camera.centerX},
        {"centerY", camera.centerY},
        {"zoom", camera.zoom},
        {"animationEnabled", camera.animationEnabled},
        {"keys", std::move(keys)},
    };
}

CameraKey cameraKeyFromJson(const json& value)
{
    CameraKey key;
    if (!value.is_object()) {
        return key;
    }
    key.frame = jsonInt(value, "frame", key.frame);
    key.centerX = jsonFloat(value, "centerX", key.centerX);
    key.centerY = jsonFloat(value, "centerY", key.centerY);
    key.zoom = jsonFloat(value, "zoom", key.zoom);
    return key;
}

CameraSettings cameraFromJson(const json& value)
{
    CameraSettings camera;
    if (!value.is_object()) {
        return camera;
    }

    camera.centerX = jsonFloat(value, "centerX", camera.centerX);
    camera.centerY = jsonFloat(value, "centerY", camera.centerY);
    camera.zoom = jsonFloat(value, "zoom", camera.zoom);
    camera.animationEnabled = value.value("animationEnabled", camera.animationEnabled);
    camera.keys.clear();

    const json keys = value.value("keys", json::array());
    if (keys.is_array()) {
        for (const json& key : keys) {
            camera.keys.push_back(cameraKeyFromJson(key));
        }
    }
    return camera;
}

json cutMetadataToJson(const Cut& cut)
{
    json cellZOrderKeys = json::array();
    for (const std::string& key : cut.cellZOrderKeys) {
        cellZOrderKeys.push_back(key);
    }

    json metadata{
        {"kind", kCutKind},
        {"formatVersion", kCutFormatVersion},
        {"id", cut.id},
        {"name", cut.name},
        {"frameRate", cut.frameRate},
        {"totalFrames", cut.totalFrames},
        {"timesheetFile", "timesheet.json"},
        {"cellZOrderKeys", std::move(cellZOrderKeys)},
    };
    if (cut.hasCamera) {
        metadata["camera"] = cameraToJson(cut.camera);
    }
    return metadata;
}

Cut cutMetadataFromJson(const json& value)
{
    Cut cut;
    cut.id = value.value("id", std::string{});
    cut.name = value.value("name", std::string{});
    cut.frameRate = value.value("frameRate", cut.frameRate);
    cut.totalFrames = value.value("totalFrames", cut.totalFrames);

    if (cut.frameRate <= 0) {
        cut.frameRate = 24;
    }
    cut.totalFrames = normalizeTimesheetFrameCount(cut.totalFrames);
    cut.timesheet.totalFrames = cut.totalFrames;

    const auto cameraIterator = value.find("camera");
    if (cameraIterator != value.end() && cameraIterator->is_object()) {
        cut.hasCamera = true;
        cut.camera = cameraFromJson(*cameraIterator);
    }

    if (const json* cellZOrderKeys = value.find("cellZOrderKeys") == value.end() ? nullptr : &value.at("cellZOrderKeys")) {
        if (cellZOrderKeys->is_array()) {
            for (const json& keyJson : *cellZOrderKeys) {
                if (keyJson.is_string()) {
                    cut.cellZOrderKeys.push_back(keyJson.get<std::string>());
                }
            }
        }
    }

    return cut;
}

} // namespace

std::filesystem::path cutJsonPathForCutFolder(const std::filesystem::path& cutFolder)
{
    return cutFolder / "cut.json";
}

std::filesystem::path timesheetJsonPathForCutFolder(const std::filesystem::path& cutFolder)
{
    return TimesheetIO::timesheetPathForCutFolder(cutFolder);
}

bool saveCut(
    const Cut& cut,
    const std::filesystem::path& cutFolder,
    std::string* errorMessage)
{
    Cut normalizedCut = cut;
    normalizedCut.totalFrames = normalizeTimesheetFrameCount(normalizedCut.totalFrames);
    normalizedCut.frameRate = normalizedCut.frameRate <= 0 ? 24 : normalizedCut.frameRate;
    normalizedCut.timesheet.totalFrames = normalizedCut.totalFrames;
    normalizeTimesheet(normalizedCut.timesheet);

    if (!writeJsonFileIfChanged(cutJsonPathForCutFolder(cutFolder), cutMetadataToJson(normalizedCut), errorMessage)) {
        return false;
    }

    if (!TimesheetIO::saveTimesheet(normalizedCut.timesheet, timesheetJsonPathForCutFolder(cutFolder), errorMessage)) {
        return false;
    }

    return true;
}

bool loadCut(
    const std::filesystem::path& cutFolder,
    Cut& cut,
    std::string* errorMessage)
{
    cut = Cut{};

    json metadata;
    if (!readJsonFile(cutJsonPathForCutFolder(cutFolder), metadata, errorMessage)) {
        cut.timesheet.totalFrames = cut.totalFrames;
        normalizeTimesheet(cut.timesheet);
        return false;
    }

    if (!metadata.is_object()) {
        setError(errorMessage, "cut json root is not an object: " + cutJsonPathForCutFolder(cutFolder).string());
        cut.timesheet.totalFrames = cut.totalFrames;
        normalizeTimesheet(cut.timesheet);
        return false;
    }

    const int formatVersion = metadata.value("formatVersion", 0);
    if (formatVersion != kCutFormatVersion) {
        setError(errorMessage, "unsupported cut formatVersion: " + std::to_string(formatVersion));
        cut.timesheet.totalFrames = cut.totalFrames;
        normalizeTimesheet(cut.timesheet);
        return false;
    }

    cut = cutMetadataFromJson(metadata);

    Timesheet loadedTimesheet;
    if (!TimesheetIO::loadTimesheet(timesheetJsonPathForCutFolder(cutFolder), loadedTimesheet, errorMessage)) {
        cut.timesheet.totalFrames = cut.totalFrames;
        normalizeTimesheet(cut.timesheet);
        return false;
    }

    loadedTimesheet.totalFrames = cut.totalFrames;
    normalizeTimesheet(loadedTimesheet);
    cut.timesheet = std::move(loadedTimesheet);
    return true;
}

} // namespace perapera::CutIO
