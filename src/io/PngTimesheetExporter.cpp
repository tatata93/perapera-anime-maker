#include "io/PngTimesheetExporter.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <future>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace perapera {
namespace {

struct ExportTaskResult {
    bool ok = false;
    std::string errorMessage;
};

void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

std::filesystem::path sequencePath(const std::filesystem::path& folder, int index)
{
    std::ostringstream name;
    name << "frame_" << std::setw(3) << std::setfill('0') << index << ".png";
    return folder / name.str();
}

Cell makeSingleFrameCell(const ResolvedTimesheetSceneCell& resolved)
{
    Cell cell = *resolved.cell;
    cell.visible = true;
    cell.placement = resolved.placement;
    cell.frames.clear();
    cell.frames.push_back(resolved.cell->frames[static_cast<std::size_t>(resolved.drawingFrameIndex)]);
    return cell;
}

bool exportResolvedFrame(const ResolvedTimesheetSceneFrame& resolved,
                         const std::filesystem::path& outputPath,
                         int width,
                         int height,
                         ExportMode mode,
                         std::string* errorMessage)
{
    if (resolved.cells.empty()) {
        const Frame blankFrame;
        return PngExporter::exportFrame(blankFrame, outputPath, width, height, mode, errorMessage);
    }

    std::vector<Cell> singleFrameCells;
    singleFrameCells.reserve(resolved.cells.size());
    for (const ResolvedTimesheetSceneCell& resolvedCell : resolved.cells) {
        if (resolvedCell.cell == nullptr || resolvedCell.drawingFrameIndex < 0) {
            continue;
        }
        singleFrameCells.push_back(makeSingleFrameCell(resolvedCell));
    }

    std::vector<const Cell*> cellPointers;
    cellPointers.reserve(singleFrameCells.size());
    for (const Cell& cell : singleFrameCells) {
        cellPointers.push_back(&cell);
    }

    if (cellPointers.empty()) {
        const Frame blankFrame;
        return PngExporter::exportFrame(blankFrame, outputPath, width, height, mode, errorMessage);
    }

    return PngExporter::exportCellsFrame(cellPointers, 0, outputPath, width, height, mode, errorMessage);
}

ExportTaskResult exportResolvedFrameTask(ResolvedTimesheetSceneFrame resolved,
                                         std::filesystem::path outputPath,
                                         int width,
                                         int height,
                                         ExportMode mode)
{
    ExportTaskResult result;
    result.ok = exportResolvedFrame(resolved, outputPath, width, height, mode, &result.errorMessage);
    return result;
}

bool waitForOldestExportTask(std::deque<std::future<ExportTaskResult>>& pendingTasks,
                             std::string* errorMessage)
{
    if (pendingTasks.empty()) {
        return true;
    }

    ExportTaskResult result = pendingTasks.front().get();
    pendingTasks.pop_front();
    if (!result.ok) {
        setError(errorMessage, result.errorMessage);
        return false;
    }
    return true;
}

} // namespace

bool PngTimesheetExporter::exportFrame(const Timesheet& timesheet,
                                       const std::vector<TimesheetSceneCellInput>& inputs,
                                       int timelineFrame,
                                       const std::filesystem::path& outputPath,
                                       int width,
                                       int height,
                                       ExportMode mode,
                                       std::string* errorMessage)
{
    const ResolvedTimesheetSceneFrame resolved =
        resolveTimesheetSceneFrame(timesheet, inputs, timelineFrame);
    return exportResolvedFrame(resolved, outputPath, width, height, mode, errorMessage);
}

bool PngTimesheetExporter::exportFrameSequence(const Timesheet& timesheet,
                                               const std::vector<TimesheetSceneCellInput>& inputs,
                                               const std::filesystem::path& outputFolder,
                                               int width,
                                               int height,
                                               ExportMode mode,
                                               std::string* errorMessage)
{
    const int frameCount = normalizeTimesheetFrameCount(timesheet.totalFrames);
    if (frameCount <= 0) {
        setError(errorMessage, "timesheet has no frames");
        return false;
    }

    const unsigned int hardwareThreads = std::max(1U, std::thread::hardware_concurrency());
    const std::size_t maxPendingWrites = static_cast<std::size_t>(std::clamp(hardwareThreads / 2U, 1U, 3U));
    std::deque<std::future<ExportTaskResult>> pendingWrites;

    for (int timelineFrame = 1; timelineFrame <= frameCount; ++timelineFrame) {
        ResolvedTimesheetSceneFrame resolved =
            resolveTimesheetSceneFrame(timesheet, inputs, timelineFrame);
        pendingWrites.push_back(std::async(std::launch::async,
                                           exportResolvedFrameTask,
                                           std::move(resolved),
                                           sequencePath(outputFolder, timelineFrame),
                                           width,
                                           height,
                                           mode));
        if (pendingWrites.size() >= maxPendingWrites && !waitForOldestExportTask(pendingWrites, errorMessage)) {
            return false;
        }
    }

    while (!pendingWrites.empty()) {
        if (!waitForOldestExportTask(pendingWrites, errorMessage)) {
            return false;
        }
    }
    return true;
}

} // namespace perapera
