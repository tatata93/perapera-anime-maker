// このファイルの役割:
// Appの保存・読み込み・PNG/MP4書き出しを実装する。
// Phase 1.5 Step 20: 通常保存時の検証リロードを外し、重い全ストローク再走査を明示チェック操作へ分離する。

#include "ui/App.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "core/CutCellTimesheetBridge.h"
#include "io/FfmpegRunner.h"
#include "io/PngExporter.h"
#include "io/PngTimesheetExporter.h"
#include "io/ProjectLayoutLoadEntry.h"
#include "io/ProjectLayoutSaveEntry.h"
#include "ui/AppProjectIOSupport.h"
#include "ui/panels/CellPanel.h"

namespace perapera {
namespace {

void appendCellIfValid(const Project& project, int cellIndex, std::vector<const Cell*>& cells)
{
    if (cellIndex < 0 || cellIndex >= static_cast<int>(project.cells.size())) {
        return;
    }
    cells.push_back(&project.cells[static_cast<std::size_t>(cellIndex)]);
}

std::vector<const Cell*> exportCellsForCurrentDisplay(const Project& project, int activeCellIndex)
{
    std::vector<const Cell*> cells;
    if (project.cells.empty()) {
        return cells;
    }

    const ui::CellDisplayMode displayMode = ui::currentCellDisplayMode();
    if (displayMode == ui::CellDisplayMode::ActiveOnly) {
        appendCellIfValid(project, activeCellIndex, cells);
        return cells;
    }

    if (displayMode == ui::CellDisplayMode::SoloSelected) {
        const int soloIndex = ui::currentSoloCellIndex();
        const int safeIndex = std::clamp(soloIndex >= 0 ? soloIndex : activeCellIndex,
                                         0,
                                         static_cast<int>(project.cells.size()) - 1);
        appendCellIfValid(project, safeIndex, cells);
        return cells;
    }

    cells.reserve(project.cells.size());
    for (const Cell& cell : project.cells) {
        if (cell.visible && cell.opacity > 0.0f) {
            cells.push_back(&cell);
        }
    }
    return cells;
}

bool hasTimesheetEntries(const Timesheet& timesheet)
{
    for (const TimesheetCellTrack& track : timesheet.tracks) {
        if (!track.entries.empty()) {
            return true;
        }
    }
    return false;
}

std::vector<TimesheetSceneCellInput> exportTimesheetInputsForCurrentDisplay(const Project& project, int activeCellIndex)
{
    std::vector<TimesheetSceneCellInput> inputs;
    if (project.cells.empty()) {
        return inputs;
    }

    const ui::CellDisplayMode displayMode = ui::currentCellDisplayMode();
    if (displayMode == ui::CellDisplayMode::ActiveOnly) {
        if (activeCellIndex >= 0 && activeCellIndex < static_cast<int>(project.cells.size())) {
            inputs.push_back(TimesheetSceneCellInput{
                &project.cells[static_cast<std::size_t>(activeCellIndex)],
                activeCellIndex,
                false,
            });
        }
        return inputs;
    }

    if (displayMode == ui::CellDisplayMode::SoloSelected) {
        const int soloIndex = ui::currentSoloCellIndex();
        const int safeIndex = std::clamp(soloIndex >= 0 ? soloIndex : activeCellIndex,
                                         0,
                                         static_cast<int>(project.cells.size()) - 1);
        inputs.push_back(TimesheetSceneCellInput{
            &project.cells[static_cast<std::size_t>(safeIndex)],
            safeIndex,
            false,
        });
        return inputs;
    }

    inputs.reserve(project.cells.size());
    for (int cellIndex = 0; cellIndex < static_cast<int>(project.cells.size()); ++cellIndex) {
        inputs.push_back(TimesheetSceneCellInput{
            &project.cells[static_cast<std::size_t>(cellIndex)],
            cellIndex,
            true,
        });
    }
    return inputs;
}

std::string exportCellScopeText(const std::vector<const Cell*>& cells)
{
    const ui::CellDisplayMode displayMode = ui::currentCellDisplayMode();
    if (displayMode == ui::CellDisplayMode::ActiveOnly) {
        return "active cell";
    }
    if (displayMode == ui::CellDisplayMode::SoloSelected) {
        return "solo cell";
    }
    return "visible cells=" + std::to_string(cells.size());
}

Scene activeSceneForProjectSave(const Project& project)
{
    Scene scene;
    scene.id = "scene_001";
    scene.name = project.name.empty() ? "Scene 1" : project.name;
    scene.cutOrder = {"cut_001"};
    return scene;
}

Cut activeCutForProjectSave(const Project& project, const Timesheet& workingTimesheet)
{
    Cut cut;
    cut.id = "cut_001";
    cut.name = "Cut 1";
    cut.frameRate = project.output.fps > 0 ? project.output.fps : 24;
    cut.totalFrames = project.timeline.totalFrames > 0 ? project.timeline.totalFrames : 1;
    cut.cellZOrderKeys = project.cellOrder;
    cut.timesheet = workingTimesheet;
    cut.timesheet.totalFrames = cut.totalFrames;
    cut.hasCamera = true;
    cut.camera = project.camera;
    (void)ensureTimesheetTracksForActiveCutCells(project, cut.timesheet);
    return cut;
}

bool saveProjectNewLayoutForActiveCut(const std::filesystem::path& projectFolder,
                                      const Project& project,
                                      const Timesheet& workingTimesheet,
                                      ProjectLayoutSaveEntryResult* result,
                                      std::string* error)
{
    const Scene scene = activeSceneForProjectSave(project);
    const Cut cut = activeCutForProjectSave(project, workingTimesheet);
    return saveProjectNewLayoutMinimal(projectFolder, scene, cut, project, result, error);
}

bool loadProjectNewLayoutForActiveCut(const std::filesystem::path& projectFolder,
                                      ProjectLayoutLoadResult& result,
                                      std::string* error)
{
    ProjectLayoutLoadOptions options;
    return loadProjectNewLayoutMinimal(projectFolder, options, &result, error);
}

void applyLoadedSceneCutSettings(Project& project, const Scene& scene, const Cut& cut)
{
    (void)scene;
    if (cut.totalFrames > 0) {
        project.timeline.totalFrames = cut.totalFrames;
    }
    if (cut.frameRate > 0) {
        project.output.fps = cut.frameRate;
    }
    if (cut.hasCamera) {
        project.camera = cut.camera;
    }
}

} // namespace

void App::saveProject()
{
    Project toSave = project_;
    appio::normalizeProjectForStep14(toSave);

    const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
    std::string error;
    ProjectLayoutSaveEntryResult newLayoutResult;
    if (!saveProjectNewLayoutForActiveCut(projectFolder, toSave, workingTimesheet_, &newLayoutResult, &error)) {
        lastMessage_ = "new layout save failed: " + error;
        return;
    }

    const appio::ProjectStats stats = appio::collectProjectStats(toSave);
    if (!appio::writeAppState(projectFolder, activeCellIndex_, activeFrameIndex_, activeLayerIndex_, stats, &error)) {
        lastMessage_ = "project saved, but app_state failed: " + error;
        return;
    }

    if (!saveColorPalette(projectFolder, &error)) {
        lastMessage_ = "project saved, but palette save failed: " + error;
        return;
    }

    // Keep the heavier reload verification behind the explicit round-trip check.
    project_ = std::move(toSave);
    afterProjectChanged();
    const appio::SavedSelection selection{activeCellIndex_, activeFrameIndex_, activeLayerIndex_, true};
    lastMessage_ = "project saved: " + projectFolder.string() + " | " +
        appio::statsToText(stats) + " | " + appio::selectionText(selection) +
        " | new layout cells=" + std::to_string(newLayoutResult.cellCount) +
        " frames=" + std::to_string(newLayoutResult.frameCount);
}

void App::loadProject()
{
    const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
    ProjectLayoutLoadResult loadResult;
    std::string error;
    if (!loadProjectNewLayoutForActiveCut(projectFolder, loadResult, &error)) {
        lastMessage_ = "load failed: " + error;
        return;
    }

    Project loaded = std::move(loadResult.project);
    applyLoadedSceneCutSettings(loaded, loadResult.scene, loadResult.cut);
    appio::normalizeProjectForStep14(loaded);
    appio::SavedSelection selection = appio::readAppState(projectFolder);
    std::string selectionSource = "app_state";
    if (!selection.hasValue || !appio::validSelection(loaded, selection)) {
        selectionSource = "first non-empty";
        if (!appio::findFirstNonEmptySelection(loaded, selection)) {
            selection = appio::SavedSelection{0, 0, 0, true};
            selectionSource = "default";
        }
    }

    if (selection.hasValue && appio::validSelection(loaded, selection) &&
        !appio::selectionFrameHasStrokes(loaded, selection)) {
        appio::SavedSelection visibleSelection;
        if (appio::findFirstNonEmptySelection(loaded, visibleSelection)) {
            selection = visibleSelection;
            selectionSource = "first non-empty visible";
        }
    }

    // ロード前の巨大ProjectをUndoへ積むと、開くだけで重くなる。
    // 別プロジェクトへの切替後に古いプロジェクトへUndoする操作も危険なので、履歴はクリアする。
    undoStack_.clear();
    redoStack_.clear();

    project_ = std::move(loaded);
    workingTimesheet_ = loadResult.cut.timesheet;
    workingTimesheetDirty_ = false;
    activeCellIndex_ = selection.cellIndex;
    activeFrameIndex_ = selection.frameIndex;
    activeLayerIndex_ = selection.layerIndex;
    afterProjectChanged();
    if (!loadColorPalette(projectFolder, &error)) {
        lastMessage_ = "project loaded, but palette load failed: " + error;
        canvasViewInitialized_ = false;
        return;
    }
    canvasViewInitialized_ = false;

    const appio::ProjectStats stats = appio::collectProjectStats(project_);
    const appio::SavedSelection active{activeCellIndex_, activeFrameIndex_, activeLayerIndex_, true};
    lastMessage_ = "project loaded: " + projectFolder.string() + " | " + appio::statsToText(stats) +
        " | " + appio::selectionText(active) + " via " + selectionSource;
}

void App::saveLoadRoundTripCheck()
{
    Project toSave = project_;
    appio::normalizeProjectForStep14(toSave);
    const appio::ProjectStats beforeStats = appio::collectProjectStats(toSave);
    const std::uint64_t beforeSignature = appio::projectSignature(toSave);
    const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);

    std::string error;
    if (!saveProjectNewLayoutForActiveCut(projectFolder, toSave, workingTimesheet_, nullptr, &error)) {
        lastMessage_ = "round trip new layout save failed: " + error;
        return;
    }

    if (!appio::writeAppState(projectFolder, activeCellIndex_, activeFrameIndex_, activeLayerIndex_, beforeStats, &error)) {
        lastMessage_ = "round trip app_state failed: " + error;
        return;
    }

    if (!saveColorPalette(projectFolder, &error)) {
        lastMessage_ = "round trip palette save failed: " + error;
        return;
    }

    ProjectLayoutLoadResult loadResult;
    if (!loadProjectNewLayoutForActiveCut(projectFolder, loadResult, &error)) {
        lastMessage_ = "round trip load failed: " + error;
        return;
    }
    Project loaded = std::move(loadResult.project);
    applyLoadedSceneCutSettings(loaded, loadResult.scene, loadResult.cut);
    appio::normalizeProjectForStep14(loaded);

    const appio::ProjectStats afterStats = appio::collectProjectStats(loaded);
    const std::uint64_t afterSignature = appio::projectSignature(loaded);
    if (appio::sameStats(beforeStats, afterStats) && beforeSignature == afterSignature) {
        appio::SavedSelection selection = appio::readAppState(projectFolder);
        if (!selection.hasValue || !appio::validSelection(loaded, selection)) {
            selection = appio::SavedSelection{activeCellIndex_, activeFrameIndex_, activeLayerIndex_, true};
        }
        project_ = std::move(loaded);
        workingTimesheet_ = loadResult.cut.timesheet;
        workingTimesheetDirty_ = false;
        activeCellIndex_ = selection.cellIndex;
        activeFrameIndex_ = selection.frameIndex;
        activeLayerIndex_ = selection.layerIndex;
        afterProjectChanged();
        if (!loadColorPalette(projectFolder, &error)) {
            lastMessage_ = "save/load check OK, but palette load failed: " + error;
            return;
        }
        lastMessage_ = "save/load check OK: " + appio::statsToText(afterStats) + " | " + appio::selectionText(selection);
    } else {
        lastMessage_ = "save/load check mismatch: before " + appio::statsToText(beforeStats) +
            " / after " + appio::statsToText(afterStats);
    }
}

void App::exportActivePng()
{
    const bool useTimesheetExport = hasTimesheetEntries(workingTimesheet_);
    if (useTimesheetExport) {
        const std::vector<TimesheetSceneCellInput> inputs =
            exportTimesheetInputsForCurrentDisplay(project_, activeCellIndex_);
        if (inputs.empty()) {
            lastMessage_ = "export failed: no export cells";
            return;
        }

        const std::filesystem::path output = appio::absolutePath(exportState_.exportFolder) / "active_frame.png";
        const int timelineFrame = clampTimesheetFrame(workingTimesheet_, timesheetPanelState_.selectedTimelineFrame + 1);
        std::string error;
        if (PngTimesheetExporter::exportFrame(workingTimesheet_,
                                              inputs,
                                              timelineFrame,
                                              output,
                                              project_.canvas.width,
                                              project_.canvas.height,
                                              exportState_.exportMode,
                                              &error)) {
            lastMessage_ = "PNG exported [timesheet T" + std::to_string(timelineFrame) + ", " +
                std::string(exportModeToString(exportState_.exportMode)) + "]: " + output.string();
        } else {
            lastMessage_ = "PNG export failed: " + error;
        }
        return;
    }

    const std::vector<const Cell*> cells = exportCellsForCurrentDisplay(project_, activeCellIndex_);
    if (cells.empty()) {
        lastMessage_ = "export failed: no export cells";
        return;
    }

    const std::filesystem::path output = appio::absolutePath(exportState_.exportFolder) / "active_frame.png";
    std::string error;
    if (PngExporter::exportCellsFrame(cells, activeFrameIndex_, output, project_.canvas.width, project_.canvas.height,
                                      exportState_.exportMode, &error)) {
        lastMessage_ = "PNG exported [" + std::string(exportModeToString(exportState_.exportMode)) + ", " +
            exportCellScopeText(cells) + "]: " + output.string();
    } else {
        lastMessage_ = "PNG export failed: " + error;
    }
}

void App::exportPngSequence()
{
    const bool useTimesheetExport = hasTimesheetEntries(workingTimesheet_);
    if (useTimesheetExport) {
        const std::vector<TimesheetSceneCellInput> inputs =
            exportTimesheetInputsForCurrentDisplay(project_, activeCellIndex_);
        if (inputs.empty()) {
            lastMessage_ = "export failed: no export cells";
            return;
        }

        const std::filesystem::path pngFolder = appio::absolutePath(exportState_.exportFolder);
        std::string error;
        if (PngTimesheetExporter::exportFrameSequence(workingTimesheet_,
                                                      inputs,
                                                      pngFolder,
                                                      project_.canvas.width,
                                                      project_.canvas.height,
                                                      exportState_.exportMode,
                                                      &error)) {
            lastMessage_ = "PNG sequence exported [timesheet, " +
                std::string(exportModeToString(exportState_.exportMode)) + "]: " + pngFolder.string();
        } else {
            lastMessage_ = "PNG sequence failed: " + error;
        }
        return;
    }

    const std::vector<const Cell*> cells = exportCellsForCurrentDisplay(project_, activeCellIndex_);
    if (cells.empty()) {
        lastMessage_ = "export failed: no export cells";
        return;
    }

    const std::filesystem::path pngFolder = appio::absolutePath(exportState_.exportFolder);
    std::string error;
    if (PngExporter::exportCellsFrameSequence(cells, pngFolder, project_.canvas.width, project_.canvas.height,
                                              exportState_.exportMode, &error)) {
        lastMessage_ = "PNG sequence exported [" + std::string(exportModeToString(exportState_.exportMode)) + "]: " +
            pngFolder.string() + " | " + exportCellScopeText(cells);
    } else {
        lastMessage_ = "PNG sequence failed: " + error;
    }
}

void App::exportMp4()
{
    const bool useTimesheetExport = hasTimesheetEntries(workingTimesheet_);
    if (useTimesheetExport) {
        const std::vector<TimesheetSceneCellInput> inputs =
            exportTimesheetInputsForCurrentDisplay(project_, activeCellIndex_);
        if (inputs.empty()) {
            lastMessage_ = "MP4 failed: no export cells";
            return;
        }

        const std::filesystem::path pngFolder = appio::absolutePath(exportState_.exportFolder);
        const std::filesystem::path mp4Output = appio::absolutePath(exportState_.mp4Path);
        const std::filesystem::path logPath = appio::mp4LogPathForOutput(mp4Output);
        appio::resetMp4PreflightLog(logPath, exportState_.ffmpegPath, pngFolder, mp4Output, project_.output.fps);
        appio::appendTextLog(logPath, "Step3-d: exporting timesheet PNG sequence before FFmpeg.\nmode=");
        appio::appendTextLog(logPath, exportModeToString(exportState_.exportMode));
        appio::appendTextLog(logPath, "\nscope=timesheet\n");

        std::string error;
        if (!PngTimesheetExporter::exportFrameSequence(workingTimesheet_,
                                                       inputs,
                                                       pngFolder,
                                                       project_.canvas.width,
                                                       project_.canvas.height,
                                                       exportState_.exportMode,
                                                       &error)) {
            appio::appendTextLog(logPath, "ERROR: timesheet PNG sequence export failed before FFmpeg.\n");
            appio::appendTextLog(logPath, "Reason: " + error + "\n");
            lastMessage_ = "MP4 failed before ffmpeg: " + error + " log: " + logPath.string();
            return;
        }

        const std::filesystem::path firstFrame = pngFolder / "frame_001.png";
        appio::appendTextLog(logPath, "Timesheet PNG sequence exported.\nExpected first frame: " + firstFrame.string() + "\n");
        const std::filesystem::path inputPattern = pngFolder / "frame_%03d.png";
        if (FfmpegRunner::pngSequenceToMp4(exportState_.ffmpegPath, inputPattern, project_.output.fps, mp4Output, &error)) {
            lastMessage_ = "MP4 exported [timesheet, " +
                std::string(exportModeToString(exportState_.exportMode)) + "]: " + mp4Output.string();
        } else {
            const std::string logText = appio::readLogForStatus(logPath);
            lastMessage_ = logText.empty() ? "MP4 failed: " + error : "MP4 failed: " + error + " | log: " + logText;
        }
        return;
    }

    const std::vector<const Cell*> cells = exportCellsForCurrentDisplay(project_, activeCellIndex_);
    if (cells.empty()) {
        lastMessage_ = "MP4 failed: no export cells";
        return;
    }

    const std::filesystem::path pngFolder = appio::absolutePath(exportState_.exportFolder);
    const std::filesystem::path mp4Output = appio::absolutePath(exportState_.mp4Path);
    const std::filesystem::path logPath = appio::mp4LogPathForOutput(mp4Output);
    appio::resetMp4PreflightLog(logPath, exportState_.ffmpegPath, pngFolder, mp4Output, project_.output.fps);
    appio::appendTextLog(logPath, "Step20: exporting PNG sequence before FFmpeg.\nmode=");
    appio::appendTextLog(logPath, exportModeToString(exportState_.exportMode));
    appio::appendTextLog(logPath, "\nscope=");
    appio::appendTextLog(logPath, exportCellScopeText(cells));
    appio::appendTextLog(logPath, "\n");

    std::string error;
    if (!PngExporter::exportCellsFrameSequence(cells, pngFolder, project_.canvas.width, project_.canvas.height,
                                               exportState_.exportMode, &error)) {
        appio::appendTextLog(logPath, "ERROR: PNG sequence export failed before FFmpeg.\n");
        appio::appendTextLog(logPath, "Reason: " + error + "\n");
        lastMessage_ = "MP4 failed before ffmpeg: " + error + " log: " + logPath.string();
        return;
    }

    const std::filesystem::path firstFrame = pngFolder / "frame_001.png";
    appio::appendTextLog(logPath, "PNG sequence exported.\nExpected first frame: " + firstFrame.string() + "\n");
    const std::filesystem::path inputPattern = pngFolder / "frame_%03d.png";
    if (FfmpegRunner::pngSequenceToMp4(exportState_.ffmpegPath, inputPattern, project_.output.fps, mp4Output, &error)) {
        lastMessage_ = "MP4 exported [" + std::string(exportModeToString(exportState_.exportMode)) + "]: " + mp4Output.string();
    } else {
        const std::string logText = appio::readLogForStatus(logPath);
        lastMessage_ = logText.empty() ? "MP4 failed: " + error : "MP4 failed: " + error + " | log: " + logText;
    }
}

} // namespace perapera
