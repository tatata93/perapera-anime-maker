#pragma once

// このファイルの役割:
// AppProjectIO.cpp の保存・読み込み補助処理を分離する。
// 500行超過を避け、パス正規化・保存検証・UI選択状態保存をまとめる。

#include <cstdint>
#include <filesystem>
#include <string>

#include "core/Project.h"

namespace perapera::appio {

struct ProjectStats {
    int cells = 0;
    int frames = 0;
    int layers = 0;
    int strokes = 0;
    int points = 0;
};

struct SavedSelection {
    int cellIndex = 0;
    int frameIndex = 0;
    int layerIndex = 0;
    bool hasValue = false;
};

std::filesystem::path stableBasePath();
std::filesystem::path absolutePath(const char* pathText);
std::filesystem::path mp4LogPathForOutput(const std::filesystem::path& outputPath);

void appendTextLog(const std::filesystem::path& logPath, const std::string& text);
std::string readLogForStatus(const std::filesystem::path& logPath);
void resetMp4PreflightLog(const std::filesystem::path& logPath,
                          const char* ffmpegPath,
                          const std::filesystem::path& pngFolder,
                          const std::filesystem::path& mp4Path,
                          int fps);

bool normalizeCellStructure(Project& project);
void normalizeProjectForStep14(Project& project);
ProjectStats collectProjectStats(const Project& project);
std::string statsToText(const ProjectStats& stats);
std::uint64_t projectSignature(const Project& project);
bool sameStats(const ProjectStats& lhs, const ProjectStats& rhs);

bool validSelection(const Project& project, const SavedSelection& selection);
bool findFirstNonEmptySelection(const Project& project, SavedSelection& selection);
bool selectionFrameHasStrokes(const Project& project, const SavedSelection& selection);
std::string selectionText(const SavedSelection& selection);

bool writeAppState(const std::filesystem::path& projectFolder,
                   int cellIndex,
                   int frameIndex,
                   int layerIndex,
                   const ProjectStats& stats,
                   std::string* error);
SavedSelection readAppState(const std::filesystem::path& projectFolder);

} // namespace perapera::appio
