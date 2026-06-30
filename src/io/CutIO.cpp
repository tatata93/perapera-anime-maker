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

json cutMetadataToJson(const Cut& cut)
{
    json cellZOrderKeys = json::array();
    for (const std::string& key : cut.cellZOrderKeys) {
        cellZOrderKeys.push_back(key);
    }

    json scenePlates = json::array();
    for (const ScenePlate& plate : cut.scenePlates.plates) {
        scenePlates.push_back(json{
            {"id", plate.id},
            {"displayName", plate.displayName},
            {"kind", scenePlateKindToString(plate.kind)},
            {"outputMode", scenePlateOutputModeToString(plate.outputMode)},
            {"imagePath", plate.imagePath},
            {"visible", plate.visible},
            {"locked", plate.locked},
            {"opacity", plate.opacity},
            {"zOrder", plate.zOrder},
            {"startTimelineFrame", plate.startTimelineFrame},
            {"endTimelineFrame", plate.endTimelineFrame},
            {"transform", {
                {"x", plate.transform.x},
                {"y", plate.transform.y},
                {"scaleX", plate.transform.scaleX},
                {"scaleY", plate.transform.scaleY},
                {"rotationDegrees", plate.transform.rotationDegrees},
            }},
        });
    }

    return json{
        {"kind", kCutKind},
        {"formatVersion", kCutFormatVersion},
        {"id", cut.id},
        {"name", cut.name},
        {"frameRate", cut.frameRate},
        {"totalFrames", cut.totalFrames},
        {"timesheetFile", "timesheet.json"},
        {"cellZOrderKeys", std::move(cellZOrderKeys)},
        {"scenePlates", std::move(scenePlates)},
    };
}

ScenePlate scenePlateFromJson(const json& value)
{
    ScenePlate plate;
    plate.id = value.value("id", std::string{});
    plate.displayName = value.value("displayName", std::string{});
    plate.kind = scenePlateKindFromString(value.value("kind", std::string{"storyboard"}));
    plate.outputMode = scenePlateOutputModeFromString(value.value("outputMode", std::string{"referenceOnly"}));
    plate.imagePath = value.value("imagePath", std::string{});
    plate.visible = value.value("visible", plate.visible);
    plate.locked = value.value("locked", plate.locked);
    plate.opacity = value.value("opacity", plate.opacity);
    plate.zOrder = value.value("zOrder", plate.zOrder);
    plate.startTimelineFrame = value.value("startTimelineFrame", plate.startTimelineFrame);
    plate.endTimelineFrame = value.value("endTimelineFrame", plate.endTimelineFrame);

    if (const json* transform = value.find("transform") == value.end() ? nullptr : &value.at("transform")) {
        if (transform->is_object()) {
            plate.transform.x = transform->value("x", plate.transform.x);
            plate.transform.y = transform->value("y", plate.transform.y);
            plate.transform.scaleX = transform->value("scaleX", plate.transform.scaleX);
            plate.transform.scaleY = transform->value("scaleY", plate.transform.scaleY);
            plate.transform.rotationDegrees = transform->value("rotationDegrees", plate.transform.rotationDegrees);
        }
    }

    normalizeScenePlate(plate);
    return plate;
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

    if (const json* cellZOrderKeys = value.find("cellZOrderKeys") == value.end() ? nullptr : &value.at("cellZOrderKeys")) {
        if (cellZOrderKeys->is_array()) {
            for (const json& keyJson : *cellZOrderKeys) {
                if (keyJson.is_string()) {
                    cut.cellZOrderKeys.push_back(keyJson.get<std::string>());
                }
            }
        }
    }

    if (const json* scenePlates = value.find("scenePlates") == value.end() ? nullptr : &value.at("scenePlates")) {
        if (scenePlates->is_array()) {
            for (const json& plateJson : *scenePlates) {
                if (plateJson.is_object()) {
                    cut.scenePlates.plates.push_back(scenePlateFromJson(plateJson));
                }
            }
        }
    }
    normalizeScenePlateStack(cut.scenePlates);

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
    normalizeScenePlateStack(normalizedCut.scenePlates);

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
