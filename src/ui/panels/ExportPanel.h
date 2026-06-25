#pragma once

// このファイルの役割:
// プロジェクト保存・読み込み、PNG/MP4書き出しUIのアクションを定義する。

#include "io/PngExporter.h"

namespace perapera::ui {

enum class ExportPanelAction {
    None,
    SaveProject,
    LoadProject,
    VerifyProjectRoundTrip,
    ExportActivePng,
    ExportPngSequence,
    ExportMp4,
};

struct ExportPanelState {
    char projectFolder[512] = "my_anime_project";
    char exportFolder[512] = "exports/png";
    char ffmpegPath[512] = "ffmpeg";
    char mp4Path[512] = "exports/mp4/output.mp4";
    ExportMode exportMode = ExportMode::Composite;
};

ExportPanelAction drawExportPanel(ExportPanelState& state, const char* lastMessage);

} // namespace perapera::ui
