#pragma once

// このファイルの役割:
// Phase 2-pre Timesheet Step B: Project直下の仮Timesheetを timesheet.json として保存・読み込みする。
// Scene/Cut移行前の互換層なので、ProjectIO本体の巨大な保存処理には深く入り込まない。

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/Project.h"

namespace perapera {
namespace TimesheetIO {

namespace detail {

using nlohmann::json;

inline constexpr int kTimesheetFormatVersion = 1;

inline void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

inline std::filesystem::path timesheetPath(const std::filesystem::path& projectFolder)
{
    return projectFolder / "timesheet.json";
}

inline json toJson(const TimesheetCellExposure& exposure)
{
    return json{
        {"cellId", exposure.cellId},
        {"drawingFrameIndex", exposure.drawingFrameIndex},
        {"kind", timesheetExposureKindToString(exposure.kind)},
    };
}

inline TimesheetCellExposure exposureFromJson(const json& value)
{
    TimesheetCellExposure exposure;
    exposure.cellId = value.value("cellId", std::string{});
    exposure.drawingFrameIndex = value.value("drawingFrameIndex", 0);
    exposure.kind = timesheetExposureKindFromString(value.value("kind", std::string{"hold"}));
    return exposure;
}

inline json toJson(const TimesheetFrame& frame)
{
    json exposures = json::array();
    for (const TimesheetCellExposure& exposure : frame.cellExposures) {
        exposures.push_back(toJson(exposure));
    }

    return json{
        {"timelineFrame", frame.timelineFrame},
        {"cellExposures", exposures},
    };
}

inline TimesheetFrame frameFromJson(const json& value, int fallbackTimelineFrame)
{
    TimesheetFrame frame;
    frame.timelineFrame = value.value("timelineFrame", fallbackTimelineFrame);

    if (value.contains("cellExposures") && value.at("cellExposures").is_array()) {
        for (const json& exposureJson : value.at("cellExposures")) {
            TimesheetCellExposure exposure = exposureFromJson(exposureJson);
            if (!exposure.cellId.empty()) {
                frame.cellExposures.push_back(std::move(exposure));
            }
        }
    }

    return frame;
}

inline const Cell* findCellById(const std::vector<Cell>& cells, const std::string& cellId)
{
    const auto it = std::find_if(cells.begin(), cells.end(), [&](const Cell& cell) {
        return cell.id == cellId;
    });
    return it == cells.end() ? nullptr : &(*it);
}

inline int maxDrawingFrameIndexForCell(const Cell& cell)
{
    if (cell.frames.empty()) {
        return 0;
    }
    return static_cast<int>(cell.frames.size()) - 1;
}

inline TimesheetCellExposure normalizedExposureForCell(
    const TimesheetFrame& frame,
    const Cell& cell,
    int defaultDrawingFrameIndex)
{
    TimesheetCellExposure normalized;
    normalized.cellId = cell.id;
    normalized.drawingFrameIndex = std::max(0, defaultDrawingFrameIndex);
    normalized.kind = TimesheetExposureKind::Hold;

    if (const TimesheetCellExposure* existing = frame.exposureForCell(cell.id)) {
        normalized = *existing;
        normalized.cellId = cell.id;
    }

    const int maxFrameIndex = maxDrawingFrameIndexForCell(cell);
    normalized.drawingFrameIndex = std::clamp(normalized.drawingFrameIndex, 0, maxFrameIndex);
    return normalized;
}

inline json timesheetToJson(const Timesheet& timesheet)
{
    json frames = json::array();
    for (const TimesheetFrame& frame : timesheet.frames) {
        frames.push_back(toJson(frame));
    }

    return json{
        {"format_version", kTimesheetFormatVersion},
        {"totalFrames", timesheet.totalFrames},
        {"frames", frames},
    };
}

inline bool writeJsonFile(const std::filesystem::path& path, const json& value, std::string* errorMessage)
{
    const std::string text = value.dump(4) + '\n';

    std::ifstream existing(path, std::ios::binary);
    if (existing) {
        std::string existingText((std::istreambuf_iterator<char>(existing)), std::istreambuf_iterator<char>());
        if (existingText == text) {
            return true;
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

inline bool readJsonFile(const std::filesystem::path& path, json& value, std::string* errorMessage)
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

inline Timesheet timesheetFromJson(const json& value, int fallbackTotalFrames)
{
    Timesheet timesheet;
    timesheet.ensureFrameCount(value.value("totalFrames", fallbackTotalFrames));

    if (value.contains("frames") && value.at("frames").is_array()) {
        for (const json& frameJson : value.at("frames")) {
            TimesheetFrame frame = frameFromJson(frameJson, 0);
            if (frame.timelineFrame < 0 || frame.timelineFrame >= static_cast<int>(timesheet.frames.size())) {
                continue;
            }
            timesheet.frames[static_cast<std::size_t>(frame.timelineFrame)] = std::move(frame);
        }
    }

    return timesheet;
}

} // namespace detail

inline void normalizeTimesheetForCells(Timesheet& timesheet, const std::vector<Cell>& cells, int totalFrames)
{
    timesheet.ensureFrameCount(totalFrames);

    for (TimesheetFrame& frame : timesheet.frames) {
        std::vector<TimesheetCellExposure> normalizedExposures;
        normalizedExposures.reserve(cells.size());

        for (const Cell& cell : cells) {
            normalizedExposures.push_back(detail::normalizedExposureForCell(frame, cell, 0));
        }

        frame.cellExposures = std::move(normalizedExposures);
    }
}

inline void normalizeProjectTimesheet(Project& project)
{
    normalizeTimesheetForCells(project.timesheet, project.cells, project.timeline.totalFrames);
}

inline bool saveProjectTimesheet(
    const Project& project,
    const std::filesystem::path& projectFolder,
    std::string* errorMessage)
{
    std::error_code errorCode;
    std::filesystem::create_directories(projectFolder, errorCode);
    if (errorCode) {
        detail::setError(errorMessage, "failed to create project folder for timesheet: " + projectFolder.string());
        return false;
    }

    Timesheet timesheet = project.timesheet;
    normalizeTimesheetForCells(timesheet, project.cells, project.timeline.totalFrames);
    return detail::writeJsonFile(detail::timesheetPath(projectFolder), detail::timesheetToJson(timesheet), errorMessage);
}

inline bool loadProjectTimesheet(
    Project& project,
    const std::filesystem::path& projectFolder,
    std::string* errorMessage)
{
    const std::filesystem::path path = detail::timesheetPath(projectFolder);
    std::error_code errorCode;
    if (!std::filesystem::exists(path, errorCode)) {
        project.timesheet = Timesheet::createDefault(project.cells, project.timeline.totalFrames);
        normalizeProjectTimesheet(project);
        return true;
    }

    detail::json value;
    if (!detail::readJsonFile(path, value, errorMessage)) {
        return false;
    }

    project.timesheet = detail::timesheetFromJson(value, project.timeline.totalFrames);
    normalizeProjectTimesheet(project);
    return true;
}

} // namespace TimesheetIO
} // namespace perapera
