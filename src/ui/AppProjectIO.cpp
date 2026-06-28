// このファイルの役割:
// Appの保存・読み込み・PNG/MP4書き出しを実装する。
// Phase 1.5 Step 20: 通常保存時の検証リロードを外し、重い全ストローク再走査を明示チェック操作へ分離する。
// Phase 2-pre Timesheet Step B: Project直下の仮Timesheetを timesheet.json として保存・読み込みする。

#include "ui/App.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "core/TimesheetResolver.h"
#include "io/FfmpegRunner.h"
#include "io/PngExporter.h"
#include "io/ProjectIO.h"
#include "io/TimesheetIO.h"
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

bool shouldExportResolvedCell(const ResolvedTimesheetCell& resolved, int activeCellIndex, int cellCount)
{
    if (resolved.cell == nullptr || resolved.frame == nullptr || !resolved.timesheetVisible || resolved.cell->opacity <= 0.0f) {
        return false;
    }

    const ui::CellDisplayMode displayMode = ui::currentCellDisplayMode();
    if (displayMode == ui::CellDisplayMode::ActiveOnly) {
        return resolved.cellIndex == activeCellIndex;
    }

    if (displayMode == ui::CellDisplayMode::SoloSelected) {
        const int soloIndex = ui::currentSoloCellIndex();
        const int safeIndex = std::clamp(soloIndex >= 0 ? soloIndex : activeCellIndex, 0, std::max(0, cellCount - 1));
        return resolved.cellIndex == safeIndex;
    }

    return resolved.renderable;
}

std::vector<ExportCellFrameRef> exportFrameRefsForCurrentDisplay(const Project& project,
                                                                 int activeCellIndex,
                                                                 int timelineFrame)
{
    std::vector<ExportCellFrameRef> refs;
    if (project.cells.empty()) {
        return refs;
    }

    TimesheetResolveOptions options;
    options.includeHiddenCells = true;
    const ResolvedTimesheetFrame resolvedFrame = TimesheetResolver::resolveFrame(project, timelineFrame, options);
    refs.reserve(resolvedFrame.cells.size());
    for (const ResolvedTimesheetCell& resolved : resolvedFrame.cells) {
        if (!shouldExportResolvedCell(resolved, activeCellIndex, static_cast<int>(project.cells.size()))) {
            continue;
        }
        refs.push_back(ExportCellFrameRef{resolved.cell, resolved.frame});
    }
    return refs;
}

std::vector<std::vector<ExportCellFrameRef>> exportFrameRefsSequenceForCurrentDisplay(const Project& project,
                                                                                     int activeCellIndex)
{
    const int frameCount = TimesheetResolver::timesheetFrameCount(project);
    std::vector<std::vector<ExportCellFrameRef>> frames;
    frames.reserve(static_cast<std::size_t>(std::max(1, frameCount)));
    for (int timelineFrame = 0; timelineFrame < frameCount; ++timelineFrame) {
        frames.push_back(exportFrameRefsForCurrentDisplay(project, activeCellIndex, timelineFrame));
    }
    return frames;
}

} // namespace

void App::saveProject()
{
    Project toSave = project_;
    appio::normalizeProjectForStep14(toSave);
    TimesheetIO::normalizeProjectTimesheet(toSave);

    const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
    std::string error;
    if (!ProjectIO::save(toSave, projectFolder, &error)) {
        lastMessage_ = "save failed: " + error;
        return;
    }

    if (!TimesheetIO::saveProjectTimesheet(toSave, projectFolder, &error)) {
        lastMessage_ = "project saved, but timesheet save failed: " + error;
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

    // Step 20:
    // 以前はここでProjectIO::load()してprojectSignature()まで走査していた。
    // FillStrokeが増えると保存のたびに全フレーム/全ストローク/全bitmapを読むため遅い。
    // 検証は saveLoadRoundTripCheck() の明示ボタンだけで行う。
    project_ = std::move(toSave);
    afterProjectChanged();
    const appio::SavedSelection selection{activeCellIndex_, activeFrameIndex_, activeLayerIndex_, true};
    lastMessage_ = "project saved: " + projectFolder.string() + " | " +
        appio::statsToText(stats) + " | " + appio::selectionText(selection);
}

void App::loadProject()
{
    const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
    Project loaded;
    std::string error;
    if (!ProjectIO::load(projectFolder, loaded, &error)) {
        lastMessage_ = "load failed: " + error;
        return;
    }

    appio::normalizeProjectForStep14(loaded);
    if (!TimesheetIO::loadProjectTimesheet(loaded, projectFolder, &error)) {
        lastMessage_ = "load failed: " + error;
        return;
    }

    appio::SavedSelection selection = appio::readAppState(projectFolder);
    std::string selectionSource = "app_state";
    if (!selection.hasValue || !appio::validSelection(loaded, selection)) {
        selectionSource = "first non-empty";
        if (!appio::findFirstNonEmptySelection(loaded, selection)) {
            selection = appio::SavedSelection{0, 0, 0, true};
            selectionSource = "default";
        }
    }

    if (selection.hasValue && appio::validSelection(loaded, selection) && !appio::selectionFrameHasStrokes(loaded, selection)) {
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
    lastMessage_ = "project loaded: " + projectFolder.string() + " | " +
        appio::statsToText(stats) + " | " + appio::selectionText(active) + " via " + selectionSource;
}

void App::saveLoadRoundTripCheck()
{
    Project toSave = project_;
    appio::normalizeProjectForStep14(toSave);
    TimesheetIO::normalizeProjectTimesheet(toSave);

    const appio::ProjectStats beforeStats = appio::collectProjectStats(toSave);
    const std::uint64_t beforeSignature = appio::projectSignature(toSave);
    const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
    std::string error;
    if (!ProjectIO::save(toSave, projectFolder, &error)) {
        lastMessage_ = "round trip save failed: " + error;
        return;
    }

    if (!TimesheetIO::saveProjectTimesheet(toSave, projectFolder, &error)) {
        lastMessage_ = "round trip timesheet save failed: " + error;
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

    Project loaded;
    if (!ProjectIO::load(projectFolder, loaded, &error)) {
        lastMessage_ = "round trip load failed: " + error;
        return;
    }
    appio::normalizeProjectForStep14(loaded);
    if (!TimesheetIO::loadProjectTimesheet(loaded, projectFolder, &error)) {
        lastMessage_ = "round trip timesheet load failed: " + error;
        return;
    }

    const appio::ProjectStats afterStats = appio::collectProjectStats(loaded);
    const std::uint64_t afterSignature = appio::projectSignature(loaded);
    if (appio::sameStats(beforeStats, afterStats) && beforeSignature == afterSignature) {
        appio::SavedSelection selection = appio::readAppState(projectFolder);
        if (!selection.hasValue || !appio::validSelection(loaded, selection)) {
            selection = appio::SavedSelection{activeCellIndex_, activeFrameIndex_, activeLayerIndex_, true};
        }

        project_ = std::move(loaded);
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
    if (project_.cells.empty()) {
        lastMessage_ = "export failed: no cells";
        return;
    }

    const std::vector<const Cell*> cells = exportCellsForCurrentDisplay(project_, activeCellIndex_);
    const std::vector<ExportCellFrameRef> refs =
        exportFrameRefsForCurrentDisplay(project_, activeCellIndex_, activeFrameIndex_);
    const std::filesystem::path output = appio::absolutePath(exportState_.exportFolder) / "active_frame.png";
    std::string error;
    if (PngExporter::exportCellFrameRefs(refs, output, project_.canvas.width, project_.canvas.height, exportState_.exportMode, &error)) {
        lastMessage_ = "PNG exported [" + std::string(exportModeToString(exportState_.exportMode)) + ", " +
            exportCellScopeText(cells) + ", timesheet T" + std::to_string(activeFrameIndex_ + 1) + "]: " + output.string();
    } else {
        lastMessage_ = "PNG export failed: " + error;
    }
}

void App::exportPngSequence()
{
    if (project_.cells.empty()) {
        lastMessage_ = "export failed: no cells";
        return;
    }

    const std::vector<const Cell*> cells = exportCellsForCurrentDisplay(project_, activeCellIndex_);
    const std::vector<std::vector<ExportCellFrameRef>> frames =
        exportFrameRefsSequenceForCurrentDisplay(project_, activeCellIndex_);
    const std::filesystem::path pngFolder = appio::absolutePath(exportState_.exportFolder);
    std::string error;
    if (PngExporter::exportCellFrameRefSequence(frames, pngFolder, project_.canvas.width, project_.canvas.height, exportState_.exportMode, &error)) {
        lastMessage_ = "PNG sequence exported [" + std::string(exportModeToString(exportState_.exportMode)) + "]: " +
            pngFolder.string() + " | " + exportCellScopeText(cells) + " | timesheet frames=" + std::to_string(frames.size());
    } else {
        lastMessage_ = "PNG sequence failed: " + error;
    }
}

void App::exportMp4()
{
    if (project_.cells.empty()) {
        lastMessage_ = "MP4 failed: no cells";
        return;
    }

    const std::vector<const Cell*> cells = exportCellsForCurrentDisplay(project_, activeCellIndex_);
    const std::vector<std::vector<ExportCellFrameRef>> frames =
        exportFrameRefsSequenceForCurrentDisplay(project_, activeCellIndex_);
    const std::filesystem::path pngFolder = appio::absolutePath(exportState_.exportFolder);
    const std::filesystem::path mp4Output = appio::absolutePath(exportState_.mp4Path);
    const std::filesystem::path logPath = appio::mp4LogPathForOutput(mp4Output);
    appio::resetMp4PreflightLog(logPath, exportState_.ffmpegPath, pngFolder, mp4Output, project_.output.fps);
    appio::appendTextLog(logPath, "Step20: exporting PNG sequence before FFmpeg.\nmode=");
    appio::appendTextLog(logPath, exportModeToString(exportState_.exportMode));
    appio::appendTextLog(logPath, "\nscope=");
    appio::appendTextLog(logPath, exportCellScopeText(cells));
    appio::appendTextLog(logPath, "\ntimesheet_frames=");
    appio::appendTextLog(logPath, std::to_string(frames.size()));
    appio::appendTextLog(logPath, "\n");

    std::string error;
    if (!PngExporter::exportCellFrameRefSequence(frames, pngFolder, project_.canvas.width, project_.canvas.height, exportState_.exportMode, &error)) {
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
