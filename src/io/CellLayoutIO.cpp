#include "io/CellLayoutIO.h"

#include <fstream>
#include <system_error>

#include <nlohmann/json.hpp>

#include "io/ProjectLayoutPaths.h"

namespace perapera {
namespace {

bool ensureDirectory(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::create_directories(path, error);
    return !error && std::filesystem::is_directory(path);
}

bool writeJsonFile(const std::filesystem::path& path, const nlohmann::json& json) {
    if (!ensureDirectory(path.parent_path())) {
        return false;
    }

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << json.dump(2) << '\n';
    return out.good();
}

nlohmann::json frameManifestJson(const Frame& frame, int frameIndex) {
    nlohmann::json json;
    json["index"] = frameIndex;
    json["name"] = frame.name;
    json["durationFrames"] = frame.durationFrames;
    json["layerCount"] = frame.layers.size();
    json["layersDirectory"] = "layers";
    return json;
}

nlohmann::json cellJson(const Cell& cell) {
    nlohmann::json json;
    json["id"] = cell.id;
    json["name"] = cell.name;
    json["category"] = cell.category;
    json["widthPx"] = cell.widthPx;
    json["heightPx"] = cell.heightPx;
    json["zOrder"] = cell.zOrder;
    json["visible"] = cell.visible;
    json["opacity"] = cell.opacity;
    json["placement"] = {
        {"x", cell.placement.x},
        {"y", cell.placement.y},
        {"scale", cell.placement.scale},
        {"rotation", cell.placement.rotation},
    };
    json["frameCount"] = cell.frames.size();
    json["framesDirectory"] = "frames";

    nlohmann::json motionKeys = nlohmann::json::array();
    for (const CellMotionKey& key : cell.motionKeys) {
        motionKeys.push_back({
            {"frame", key.frame},
            {"placement",
             {{"x", key.placement.x},
              {"y", key.placement.y},
              {"scale", key.placement.scale},
              {"rotation", key.placement.rotation}}},
            {"interpolation", key.interpolation},
        });
    }
    json["motionKeys"] = std::move(motionKeys);

    nlohmann::json frames = nlohmann::json::array();
    for (std::size_t index = 0; index < cell.frames.size(); ++index) {
        const Frame& frame = cell.frames[index];
        const int frameIndex = static_cast<int>(index);
        frames.push_back({
            {"index", frameIndex},
            {"name", frame.name},
            {"durationFrames", frame.durationFrames},
            {"path", std::string("frames/") + frameDirectoryName(frameIndex) + "/frame.json"},
        });
    }
    json["frames"] = frames;
    return json;
}

} // namespace

bool saveCellLayoutMinimal(const std::filesystem::path& projectRoot,
                           const Cell& cell,
                           const CellLayoutSaveOptions& options,
                           CellLayoutSaveSummary* outSummary) {
    const std::string sceneId = normalizeLayoutId(options.sceneId, defaultSceneId());
    const std::string cutId = normalizeLayoutId(options.cutId, defaultCutId());
    const std::string cellId = normalizeLayoutId(cell.id, "cell_001");

    const std::filesystem::path cellDir = cellDirectory(projectRoot, sceneId, cutId, cellId);
    const std::filesystem::path framesDir = framesDirectory(projectRoot, sceneId, cutId, cellId);

    if (!ensureDirectory(framesDir)) {
        return false;
    }

    if (!writeJsonFile(cellJsonPath(projectRoot, sceneId, cutId, cellId), cellJson(cell))) {
        return false;
    }

    if (options.writeFrameManifests) {
        for (std::size_t index = 0; index < cell.frames.size(); ++index) {
            const int frameIndex = static_cast<int>(index);
            const std::filesystem::path frameDir = frameDirectory(projectRoot, sceneId, cutId, cellId, frameIndex);
            if (!ensureDirectory(frameDir / "layers")) {
                return false;
            }
            if (!writeJsonFile(frameJsonPath(projectRoot, sceneId, cutId, cellId, frameIndex),
                               frameManifestJson(cell.frames[index], frameIndex))) {
                return false;
            }
        }
    }

    if (outSummary != nullptr) {
        outSummary->cellDirectory = cellDir;
        outSummary->cellJson = cellJsonPath(projectRoot, sceneId, cutId, cellId);
        outSummary->framesDirectory = framesDir;
        outSummary->frameCount = cell.frames.size();
    }
    return true;
}

bool saveCellsLayoutMinimal(const std::filesystem::path& projectRoot,
                            const std::vector<Cell>& cells,
                            const CellLayoutSaveOptions& options,
                            std::vector<CellLayoutSaveSummary>* outSummaries) {
    if (outSummaries != nullptr) {
        outSummaries->clear();
    }

    for (const Cell& cell : cells) {
        CellLayoutSaveSummary summary;
        if (!saveCellLayoutMinimal(projectRoot, cell, options, &summary)) {
            return false;
        }
        if (outSummaries != nullptr) {
            outSummaries->push_back(summary);
        }
    }
    return true;
}

} // namespace perapera
