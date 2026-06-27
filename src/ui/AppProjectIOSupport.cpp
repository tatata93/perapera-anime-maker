// このファイルの役割:
// 保存・読み込み補助処理の実装。
// relative path の基準を安定させ、再起動後も同じ保存先を見に行く。

#include "ui/AppProjectIOSupport.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace perapera::appio {
namespace {

using nlohmann::json;

void hashCombine(std::uint64_t& seed, std::uint64_t value)
{
    seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}

bool frameHasStrokes(const Frame& frame)
{
    for (const Layer& layer : frame.layers) {
        if (!layer.strokes.empty()) {
            return true;
        }
    }
    return false;
}

int firstLayerWithStrokes(const Frame& frame)
{
    for (int index = 0; index < static_cast<int>(frame.layers.size()); ++index) {
        if (!frame.layers[static_cast<std::size_t>(index)].strokes.empty()) {
            return index;
        }
    }
    return 0;
}

std::string sanitizeIdComponent(const std::string& value, const std::string& fallback)
{
    std::string out;
    out.reserve(value.size());
    for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
            out.push_back(static_cast<char>(std::tolower(ch)));
        } else if (ch == '_' || ch == '-') {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        out = fallback;
    }
    return out;
}

std::string numberedId(const char* prefix, int number)
{
    char buffer[48]{};
    std::snprintf(buffer, sizeof(buffer), "%s_%03d", prefix, std::max(1, number));
    return std::string(buffer);
}

bool stringListContains(const std::vector<std::string>& values, const std::string& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

std::string uniqueCellId(const std::vector<std::string>& usedIds, int preferredNumber)
{
    int number = std::max(1, preferredNumber);
    std::string id = numberedId("cell", number);
    while (stringListContains(usedIds, id)) {
        ++number;
        id = numberedId("cell", number);
    }
    return id;
}

std::string expectedLayerIdForCellFrame(const Cell& cell, int cellIndex, int frameIndex, int layerIndex)
{
    const std::string fallbackCell = numberedId("cell", cellIndex + 1);
    const std::string cellKey = sanitizeIdComponent(cell.id, fallbackCell);

    char buffer[96]{};
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%s_f%03d_layer_%03d",
                  cellKey.c_str(),
                  std::max(1, frameIndex + 1),
                  std::max(1, layerIndex + 1));
    return std::string(buffer);
}

std::filesystem::path appStatePath(const std::filesystem::path& projectFolder)
{
    return projectFolder / "app_state.json";
}

bool writeTextFileIfChanged(const std::filesystem::path& path,
                            const std::string& text,
                            std::string* error)
{
    std::ifstream existing(path, std::ios::binary);
    if (existing) {
        std::ostringstream buffer;
        buffer << existing.rdbuf();
        if (buffer.str() == text) {
            return true;
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        if (error != nullptr) {
            *error = "failed to write file: " + path.string();
        }
        return false;
    }
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!file) {
        if (error != nullptr) {
            *error = "failed to write file: " + path.string();
        }
        return false;
    }
    return true;
}

} // namespace

std::filesystem::path stableBasePath()
{
    std::error_code errorCode;
    std::filesystem::path probe = std::filesystem::current_path(errorCode);
    if (errorCode || probe.empty()) {
        return std::filesystem::path(".");
    }

    for (int depth = 0; depth < 8; ++depth) {
        if (std::filesystem::exists(probe / "CMakeLists.txt", errorCode) &&
            std::filesystem::exists(probe / "src", errorCode)) {
            return probe;
        }
        if (!probe.has_parent_path() || probe == probe.parent_path()) {
            break;
        }
        probe = probe.parent_path();
    }

    return std::filesystem::current_path(errorCode);
}

std::filesystem::path absolutePath(const char* pathText)
{
    std::filesystem::path path = pathText == nullptr || pathText[0] == '\0'
        ? std::filesystem::path(".")
        : std::filesystem::path(pathText);
    if (path.is_relative()) {
        path = stableBasePath() / path;
    }

    std::error_code errorCode;
    const std::filesystem::path absolute = std::filesystem::absolute(path, errorCode);
    return errorCode ? path.lexically_normal() : absolute.lexically_normal();
}

std::filesystem::path mp4LogPathForOutput(const std::filesystem::path& outputPath)
{
    const std::filesystem::path parent = outputPath.parent_path().empty()
        ? stableBasePath()
        : outputPath.parent_path();
    return parent / "ffmpeg_last_run.log";
}

void appendTextLog(const std::filesystem::path& logPath, const std::string& text)
{
    std::error_code errorCode;
    std::filesystem::create_directories(logPath.parent_path(), errorCode);
    std::ofstream log(logPath, std::ios::binary | std::ios::app);
    if (log) {
        log << text;
    }
}

std::string readLogForStatus(const std::filesystem::path& logPath)
{
    std::ifstream file(logPath, std::ios::binary);
    if (!file) {
        return {};
    }
    std::string text;
    text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (text.size() > 1400U) {
        text = text.substr(text.size() - 1400U);
    }
    std::replace(text.begin(), text.end(), '\r', ' ');
    std::replace(text.begin(), text.end(), '\n', ' ');
    return text;
}

void resetMp4PreflightLog(const std::filesystem::path& logPath,
                          const char* ffmpegPath,
                          const std::filesystem::path& pngFolder,
                          const std::filesystem::path& mp4Path,
                          int fps)
{
    std::error_code errorCode;
    std::filesystem::create_directories(logPath.parent_path(), errorCode);
    std::ofstream log(logPath, std::ios::binary | std::ios::trunc);
    if (!log) {
        return;
    }

    log << "perapera-anime-maker MP4 preflight log v12\n";
    log << "======================================\n\n";
    log << "currentPath: " << std::filesystem::current_path(errorCode).string() << "\n";
    log << "stableBase: " << stableBasePath().string() << "\n";
    log << "ffmpegPath: " << (ffmpegPath == nullptr ? "" : ffmpegPath) << "\n";
    log << "pngFolder: " << pngFolder.string() << "\n";
    log << "mp4Path: " << mp4Path.string() << "\n";
    log << "fps: " << fps << "\n\n";
}

bool normalizeCellStructure(Project& project)
{
    bool changed = false;
    if (project.cells.empty()) {
        project = Project::createDefault();
        changed = true;
    }

    project.cellOrder.clear();
    project.cellOrder.reserve(project.cells.size());

    std::vector<std::string> usedCellIds;
    usedCellIds.reserve(project.cells.size());

    for (int cellIndex = 0; cellIndex < static_cast<int>(project.cells.size()); ++cellIndex) {
        Cell& cell = project.cells[static_cast<std::size_t>(cellIndex)];
        if (cell.id.empty() || stringListContains(usedCellIds, cell.id)) {
            cell.id = uniqueCellId(usedCellIds, cellIndex + 1);
            changed = true;
        }
        usedCellIds.push_back(cell.id);

        if (cell.zOrder != cellIndex) {
            cell.zOrder = cellIndex;
            changed = true;
        }
        project.cellOrder.push_back(cell.id);
        if (cell.frames.empty()) {
            cell.frames.push_back(Frame::createDefault(1));
            changed = true;
        }
        for (int frameIndex = 0; frameIndex < static_cast<int>(cell.frames.size()); ++frameIndex) {
            Frame& frame = cell.frames[static_cast<std::size_t>(frameIndex)];
            if (frame.durationFrames < 1) {
                frame.durationFrames = 1;
                changed = true;
            }
            if (frame.layers.empty()) {
                frame.layers.push_back(Layer::createDefault(1));
                changed = true;
            }
            for (int layerIndex = 0; layerIndex < static_cast<int>(frame.layers.size()); ++layerIndex) {
                Layer& layer = frame.layers[static_cast<std::size_t>(layerIndex)];
                const std::string expectedLayerId = expectedLayerIdForCellFrame(cell, cellIndex, frameIndex, layerIndex);
                if (layer.layerId != expectedLayerId) {
                    layer.layerId = expectedLayerId;
                    layer.touchRevision();
                    changed = true;
                }
            }
        }
    }
    return changed;
}

void normalizeProjectForStep14(Project& project)
{
    (void)normalizeCellStructure(project);
}

ProjectStats collectProjectStats(const Project& project)
{
    ProjectStats stats;
    stats.cells = static_cast<int>(project.cells.size());
    for (const Cell& cell : project.cells) {
        stats.frames += static_cast<int>(cell.frames.size());
        for (const Frame& frame : cell.frames) {
            stats.layers += static_cast<int>(frame.layers.size());
            for (const Layer& layer : frame.layers) {
                stats.strokes += static_cast<int>(layer.strokes.size());
                for (const Stroke& stroke : layer.strokes) {
                    stats.points += static_cast<int>(stroke.points.size());
                }
            }
        }
    }
    return stats;
}

std::string statsToText(const ProjectStats& stats)
{
    std::ostringstream stream;
    stream << "cells=" << stats.cells
           << " frames=" << stats.frames
           << " layers=" << stats.layers
           << " strokes=" << stats.strokes
           << " points=" << stats.points;
    return stream.str();
}

std::uint64_t projectSignature(const Project& project)
{
    std::uint64_t seed = 1469598103934665603ULL;
    hashCombine(seed, static_cast<std::uint64_t>(project.cells.size()));
    for (const Cell& cell : project.cells) {
        hashCombine(seed, std::hash<std::string>{}(cell.id));
        hashCombine(seed, static_cast<std::uint64_t>(cell.frames.size()));
        for (const Frame& frame : cell.frames) {
            hashCombine(seed, std::hash<std::string>{}(frame.name));
            hashCombine(seed, static_cast<std::uint64_t>(frame.layers.size()));
            for (const Layer& layer : frame.layers) {
                hashCombine(seed, std::hash<std::string>{}(layer.layerId));
                hashCombine(seed, static_cast<std::uint64_t>(layer.strokes.size()));
                for (const Stroke& stroke : layer.strokes) {
                    hashCombine(seed, static_cast<std::uint64_t>(stroke.points.size()));
                    for (const StrokePoint& point : stroke.points) {
                        hashCombine(seed, static_cast<std::uint64_t>(static_cast<std::int64_t>(point.x * 1000.0f)));
                        hashCombine(seed, static_cast<std::uint64_t>(static_cast<std::int64_t>(point.y * 1000.0f)));
                    }
                }
            }
        }
    }
    return seed;
}

bool sameStats(const ProjectStats& lhs, const ProjectStats& rhs)
{
    return lhs.cells == rhs.cells && lhs.frames == rhs.frames && lhs.layers == rhs.layers &&
        lhs.strokes == rhs.strokes && lhs.points == rhs.points;
}

bool validSelection(const Project& project, const SavedSelection& selection)
{
    if (selection.cellIndex < 0 || selection.cellIndex >= static_cast<int>(project.cells.size())) {
        return false;
    }
    const Cell& cell = project.cells[static_cast<std::size_t>(selection.cellIndex)];
    if (selection.frameIndex < 0 || selection.frameIndex >= static_cast<int>(cell.frames.size())) {
        return false;
    }
    const Frame& frame = cell.frames[static_cast<std::size_t>(selection.frameIndex)];
    return selection.layerIndex >= 0 && selection.layerIndex < static_cast<int>(frame.layers.size());
}

bool findFirstNonEmptySelection(const Project& project, SavedSelection& selection)
{
    for (int cellIndex = 0; cellIndex < static_cast<int>(project.cells.size()); ++cellIndex) {
        const Cell& cell = project.cells[static_cast<std::size_t>(cellIndex)];
        for (int frameIndex = 0; frameIndex < static_cast<int>(cell.frames.size()); ++frameIndex) {
            const Frame& frame = cell.frames[static_cast<std::size_t>(frameIndex)];
            if (frameHasStrokes(frame)) {
                selection = SavedSelection{cellIndex, frameIndex, firstLayerWithStrokes(frame), true};
                return true;
            }
        }
    }
    return false;
}

bool selectionFrameHasStrokes(const Project& project, const SavedSelection& selection)
{
    if (!validSelection(project, selection)) {
        return false;
    }
    const Cell& cell = project.cells[static_cast<std::size_t>(selection.cellIndex)];
    const Frame& frame = cell.frames[static_cast<std::size_t>(selection.frameIndex)];
    return frameHasStrokes(frame);
}

std::string selectionText(const SavedSelection& selection)
{
    std::ostringstream stream;
    stream << "selected=" << (selection.cellIndex + 1) << "/" << (selection.frameIndex + 1)
           << "/" << (selection.layerIndex + 1);
    return stream.str();
}

bool writeAppState(const std::filesystem::path& projectFolder,
                   int cellIndex,
                   int frameIndex,
                   int layerIndex,
                   const ProjectStats& stats,
                   std::string* error)
{
    const json state = {
        {"version", 1},
        {"activeCellIndex", cellIndex},
        {"activeFrameIndex", frameIndex},
        {"activeLayerIndex", layerIndex},
        {"stats", {{"cells", stats.cells}, {"frames", stats.frames}, {"layers", stats.layers},
                   {"strokes", stats.strokes}, {"points", stats.points}}},
    };

    std::error_code errorCode;
    std::filesystem::create_directories(projectFolder, errorCode);
    if (errorCode) {
        if (error != nullptr) {
            *error = "failed to write app_state.json: " + appStatePath(projectFolder).string();
        }
        return false;
    }
    const std::string text = state.dump(4) + '\n';
    return writeTextFileIfChanged(appStatePath(projectFolder), text, error);
}

SavedSelection readAppState(const std::filesystem::path& projectFolder)
{
    SavedSelection selection;
    std::ifstream file(appStatePath(projectFolder), std::ios::binary);
    if (!file) {
        return selection;
    }

    try {
        json state;
        file >> state;
        selection.cellIndex = state.value("activeCellIndex", 0);
        selection.frameIndex = state.value("activeFrameIndex", 0);
        selection.layerIndex = state.value("activeLayerIndex", 0);
        selection.hasValue = true;
    } catch (const std::exception&) {
        selection.hasValue = false;
    }
    return selection;
}

} // namespace perapera::appio
