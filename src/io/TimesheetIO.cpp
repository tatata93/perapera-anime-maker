// このファイルの役割:
// 正式タイムシートv1を JSON ファイルとして保存・読み込みする。
// ここではProject保存とは接続せず、core Timesheet の往復だけを検証可能にする。

#include "io/TimesheetIO.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

namespace perapera::TimesheetIO {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

constexpr int kTimesheetFormatVersion = 1;
constexpr const char* kTimesheetKind = "perapera.timesheet.v1";

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
            setError(errorMessage, "failed to create folder for timesheet: " + parent.string());
            return false;
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(errorMessage, "failed to open timesheet for write: " + path.string());
        return false;
    }

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!file) {
        setError(errorMessage, "failed to write timesheet json: " + path.string());
        return false;
    }

    return true;
}

bool readJsonFile(const fs::path& path, json& value, std::string* errorMessage)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        setError(errorMessage, "failed to open timesheet for read: " + path.string());
        return false;
    }

    try {
        file >> value;
    } catch (const std::exception& exception) {
        setError(errorMessage, "failed to parse timesheet json: " + path.string() + " / " + exception.what());
        return false;
    }

    return true;
}

json entryToJson(const TimesheetEntry& entry)
{
    json value = {
        {"timelineFrame", entry.timelineFrame},
        {"type", timesheetEntryTypeToString(entry.type)},
    };

    if (timesheetEntryTypeUsesDrawingNumber(entry.type)) {
        value["drawingFrameNumber"] = entry.drawingFrameNumber;
    }

    if (!entry.note.empty()) {
        value["note"] = entry.note;
    }

    return value;
}

TimesheetEntry entryFromJson(const json& value)
{
    TimesheetEntry entry;
    entry.timelineFrame = value.value("timelineFrame", 1);
    entry.type = timesheetEntryTypeFromString(value.value("type", std::string{"hold"}));
    entry.drawingFrameNumber = value.value("drawingFrameNumber", 0);
    entry.note = value.value("note", std::string{});
    return entry;
}

json trackToJson(const TimesheetCellTrack& track)
{
    json entries = json::array();
    for (const TimesheetEntry& entry : track.entries) {
        entries.push_back(entryToJson(entry));
    }

    return json{
        {"cellId", track.cellId},
        {"displayName", track.displayName},
        {"defaultExposure", track.defaultExposure},
        {"entries", std::move(entries)},
    };
}

TimesheetCellTrack trackFromJson(const json& value)
{
    TimesheetCellTrack track;
    track.cellId = value.value("cellId", std::string{});
    track.displayName = value.value("displayName", std::string{});
    track.defaultExposure = value.value("defaultExposure", 1);

    if (const json* entries = value.find("entries") == value.end() ? nullptr : &value.at("entries")) {
        if (entries->is_array()) {
            for (const json& entryJson : *entries) {
                if (entryJson.is_object()) {
                    track.entries.push_back(entryFromJson(entryJson));
                }
            }
        }
    }

    return track;
}

json frameNoteToJson(const TimesheetFrameNote& note)
{
    json value = {
        {"timelineFrame", note.timelineFrame},
    };

    if (!note.dialogue.empty()) {
        value["dialogue"] = note.dialogue;
    }
    if (!note.cameraInstruction.empty()) {
        value["cameraInstruction"] = note.cameraInstruction;
    }
    if (!note.shootingInstruction.empty()) {
        value["shootingInstruction"] = note.shootingInstruction;
    }
    if (!note.materialMemo.empty()) {
        value["materialMemo"] = note.materialMemo;
    }

    return value;
}

TimesheetFrameNote frameNoteFromJson(const json& value)
{
    TimesheetFrameNote note;
    note.timelineFrame = value.value("timelineFrame", 1);
    note.dialogue = value.value("dialogue", std::string{});
    note.cameraInstruction = value.value("cameraInstruction", std::string{});
    note.shootingInstruction = value.value("shootingInstruction", std::string{});
    note.materialMemo = value.value("materialMemo", std::string{});
    return note;
}

json timesheetToJson(Timesheet timesheet)
{
    normalizeTimesheet(timesheet);

    json tracks = json::array();
    for (const TimesheetCellTrack& track : timesheet.tracks) {
        tracks.push_back(trackToJson(track));
    }

    json frameNotes = json::array();
    for (const TimesheetFrameNote& note : timesheet.frameNotes) {
        frameNotes.push_back(frameNoteToJson(note));
    }

    return json{
        {"kind", kTimesheetKind},
        {"formatVersion", kTimesheetFormatVersion},
        {"totalFrames", timesheet.totalFrames},
        {"defaultExposure", timesheet.defaultExposure},
        {"tracks", std::move(tracks)},
        {"frameNotes", std::move(frameNotes)},
    };
}

Timesheet timesheetFromJson(const json& value)
{
    Timesheet timesheet;
    timesheet.totalFrames = value.value("totalFrames", timesheet.totalFrames);
    timesheet.defaultExposure = value.value("defaultExposure", timesheet.defaultExposure);

    if (const json* tracks = value.find("tracks") == value.end() ? nullptr : &value.at("tracks")) {
        if (tracks->is_array()) {
            for (const json& trackJson : *tracks) {
                if (!trackJson.is_object()) {
                    continue;
                }
                TimesheetCellTrack track = trackFromJson(trackJson);
                if (!track.cellId.empty()) {
                    timesheet.tracks.push_back(std::move(track));
                }
            }
        }
    }

    if (const json* frameNotes = value.find("frameNotes") == value.end() ? nullptr : &value.at("frameNotes")) {
        if (frameNotes->is_array()) {
            for (const json& noteJson : *frameNotes) {
                if (noteJson.is_object()) {
                    timesheet.frameNotes.push_back(frameNoteFromJson(noteJson));
                }
            }
        }
    }

    normalizeTimesheet(timesheet);
    return timesheet;
}

} // namespace

std::filesystem::path timesheetPathForCutFolder(const std::filesystem::path& cutFolder)
{
    return cutFolder / "timesheet.json";
}

bool saveTimesheet(
    const Timesheet& timesheet,
    const std::filesystem::path& path,
    std::string* errorMessage)
{
    return writeJsonFileIfChanged(path, timesheetToJson(timesheet), errorMessage);
}

bool loadTimesheet(
    const std::filesystem::path& path,
    Timesheet& timesheet,
    std::string* errorMessage)
{
    timesheet = Timesheet{};

    json value;
    if (!readJsonFile(path, value, errorMessage)) {
        normalizeTimesheet(timesheet);
        return false;
    }

    if (!value.is_object()) {
        setError(errorMessage, "timesheet json root is not an object: " + path.string());
        normalizeTimesheet(timesheet);
        return false;
    }

    const int formatVersion = value.value("formatVersion", 0);
    if (formatVersion != kTimesheetFormatVersion) {
        setError(errorMessage, "unsupported timesheet formatVersion: " + std::to_string(formatVersion));
        normalizeTimesheet(timesheet);
        return false;
    }

    timesheet = timesheetFromJson(value);
    return true;
}

} // namespace perapera::TimesheetIO
