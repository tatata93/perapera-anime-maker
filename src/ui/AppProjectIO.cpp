// このファイルの役割:
// Appの保存・読み込み・PNG/MP4書き出しを実装する。
// v12ではProjectIOの保存データを読み直して検証し、再起動後の選択状態も復元する。

#include "ui/App.h"

#include <filesystem>
#include <string>
#include <utility>

#include "io/FfmpegRunner.h"
#include "io/PngExporter.h"
#include "io/ProjectIO.h"
#include "ui/AppProjectIOSupport.h"

namespace perapera {

void App::saveProject()
{
    Project toSave = project_;
    appio::normalizeProjectForStep14(toSave);

    const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
    std::string error;
    if (!ProjectIO::save(toSave, projectFolder, &error)) {
        lastMessage_ = "save failed: " + error;
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

    Project reloaded;
    if (!ProjectIO::load(projectFolder, reloaded, &error)) {
        lastMessage_ = "save verify load failed: " + error;
        return;
    }
    appio::normalizeProjectForStep14(reloaded);
    const appio::ProjectStats reloadedStats = appio::collectProjectStats(reloaded);
    if (!appio::sameStats(stats, reloadedStats) || appio::projectSignature(toSave) != appio::projectSignature(reloaded)) {
        lastMessage_ = "save verify mismatch: saved " + appio::statsToText(stats) +
            " / loaded " + appio::statsToText(reloadedStats);
        return;
    }

    project_ = std::move(toSave);
    afterProjectChanged();
    const appio::SavedSelection selection{activeCellIndex_, activeFrameIndex_, activeLayerIndex_, true};
    lastMessage_ = "project saved+verified: " + projectFolder.string() + " | " +
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

    pushUndoSnapshot();
    project_ = std::move(loaded);
    activeCellIndex_ = selection.cellIndex;
    activeFrameIndex_ = selection.frameIndex;
    activeLayerIndex_ = selection.layerIndex;
    redoStack_.clear();
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
    if (!ProjectIO::save(toSave, projectFolder, &error)) {
        lastMessage_ = "round trip save failed: " + error;
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
    const Frame* frame = activeFrame();
    if (frame == nullptr) {
        lastMessage_ = "export failed: no active frame";
        return;
    }

    const std::filesystem::path output = appio::absolutePath(exportState_.exportFolder) / "active_frame.png";
    std::string error;
    if (PngExporter::exportFrame(*frame, output, project_.canvas.width, project_.canvas.height,
                                  exportState_.exportMode, &error)) {
        lastMessage_ = "PNG exported [" + std::string(exportModeToString(exportState_.exportMode)) +
            "]: " + output.string();
    } else {
        lastMessage_ = "PNG export failed: " + error;
    }
}

void App::exportPngSequence()
{
    const Cell* cell = activeCell();
    if (cell == nullptr) {
        lastMessage_ = "export failed: no active cell";
        return;
    }

    const std::filesystem::path pngFolder = appio::absolutePath(exportState_.exportFolder);
    std::string error;
    if (PngExporter::exportFrameSequence(*cell, pngFolder, project_.canvas.width, project_.canvas.height,
                                          exportState_.exportMode, &error)) {
        lastMessage_ = "PNG sequence exported [" + std::string(exportModeToString(exportState_.exportMode)) +
            "]: " + pngFolder.string();
    } else {
        lastMessage_ = "PNG sequence failed: " + error;
    }
}

void App::exportMp4()
{
    const Cell* cell = activeCell();
    if (cell == nullptr) {
        lastMessage_ = "MP4 failed: no active cell";
        return;
    }

    const std::filesystem::path pngFolder = appio::absolutePath(exportState_.exportFolder);
    const std::filesystem::path mp4Output = appio::absolutePath(exportState_.mp4Path);
    const std::filesystem::path logPath = appio::mp4LogPathForOutput(mp4Output);

    appio::resetMp4PreflightLog(logPath, exportState_.ffmpegPath, pngFolder, mp4Output, project_.output.fps);
    appio::appendTextLog(logPath, "Step15: exporting PNG sequence before FFmpeg. mode=");
    appio::appendTextLog(logPath, exportModeToString(exportState_.exportMode));
    appio::appendTextLog(logPath, "\n");

    std::string error;
    if (!PngExporter::exportFrameSequence(*cell, pngFolder, project_.canvas.width, project_.canvas.height,
                                          exportState_.exportMode, &error)) {
        appio::appendTextLog(logPath, "ERROR: PNG sequence export failed before FFmpeg.\n");
        appio::appendTextLog(logPath, "Reason: " + error + "\n");
        lastMessage_ = "MP4 failed before ffmpeg: " + error + " log: " + logPath.string();
        return;
    }

    const std::filesystem::path firstFrame = pngFolder / "frame_001.png";
    appio::appendTextLog(logPath, "PNG sequence exported. Expected first frame: " + firstFrame.string() + "\n");

    const std::filesystem::path inputPattern = pngFolder / "frame_%03d.png";
    if (FfmpegRunner::pngSequenceToMp4(exportState_.ffmpegPath, inputPattern, project_.output.fps, mp4Output, &error)) {
        lastMessage_ = "MP4 exported [" + std::string(exportModeToString(exportState_.exportMode)) +
            "]: " + mp4Output.string();
    } else {
        const std::string logText = appio::readLogForStatus(logPath);
        lastMessage_ = logText.empty() ? "MP4 failed: " + error : "MP4 failed: " + error + " | log: " + logText;
    }
}

} // namespace perapera
