#include "io/SceneCutLayoutIO.h"

#include "io/CutIO.h"
#include "io/ProjectLayoutPaths.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <system_error>

namespace perapera::SceneCutLayoutIO {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

constexpr int kSceneFormatVersion = 1;
constexpr const char* kSceneKind = "perapera.scene.v1";

void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

std::string normalizedId(const std::string& id, const char* fallback)
{
    return ProjectLayoutPaths::sanitizeId(id, fallback);
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
            setError(errorMessage, "failed to create scene json folder: " + parent.string());
            return false;
        }
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(errorMessage, "failed to open scene json for write: " + path.string());
        return false;
    }

    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!file) {
        setError(errorMessage, "failed to write scene json: " + path.string());
        return false;
    }

    return true;
}

json sceneMetadataToJson(const Scene& scene, const Cut& cut)
{
    const std::string sceneId = normalizedId(scene.id, "scene_001");
    const std::string cutId = normalizedId(cut.id, "cut_001");

    json cuts = json::array();
    cuts.push_back(json{{"id", cutId}, {"name", cut.name.empty() ? cutId : cut.name}, {"folder", "cuts/" + cutId}});

    json cutOrder = json::array();
    cutOrder.push_back(cutId);

    return json{
        {"kind", kSceneKind},
        {"formatVersion", kSceneFormatVersion},
        {"id", sceneId},
        {"name", scene.name.empty() ? sceneId : scene.name},
        {"memo", scene.memo},
        {"cuts", std::move(cuts)},
        {"cutOrder", std::move(cutOrder)},
    };
}

} // namespace

bool saveMinimalSceneAndCut(
    const std::filesystem::path& projectRoot,
    const Scene& scene,
    const Cut& cut,
    SceneCutLayoutSaveResult* result,
    std::string* errorMessage)
{
    Scene normalizedScene = scene;
    Cut normalizedCut = cut;

    normalizedScene.id = normalizedId(normalizedScene.id, "scene_001");
    normalizedCut.id = normalizedId(normalizedCut.id, "cut_001");
    if (normalizedScene.name.empty()) {
        normalizedScene.name = normalizedScene.id;
    }
    if (normalizedCut.name.empty()) {
        normalizedCut.name = normalizedCut.id;
    }

    std::error_code errorCode;
    if (!ProjectLayoutPaths::createCutFolderSkeleton(projectRoot, normalizedScene.id, normalizedCut.id, errorCode)) {
        setError(errorMessage, "failed to create scene/cut folders: " + errorCode.message());
        return false;
    }

    const fs::path scenePath = ProjectLayoutPaths::sceneJsonPath(projectRoot, normalizedScene.id);
    const fs::path cutFolder = ProjectLayoutPaths::cutFolder(projectRoot, normalizedScene.id, normalizedCut.id);

    if (!writeJsonFileIfChanged(scenePath, sceneMetadataToJson(normalizedScene, normalizedCut), errorMessage)) {
        return false;
    }

    if (!CutIO::saveCut(normalizedCut, cutFolder, errorMessage)) {
        return false;
    }

    if (result != nullptr) {
        result->sceneJsonPath = scenePath;
        result->cutFolder = cutFolder;
        result->cutJsonPath = CutIO::cutJsonPathForCutFolder(cutFolder);
        result->timesheetJsonPath = CutIO::timesheetJsonPathForCutFolder(cutFolder);
    }

    return true;
}

} // namespace perapera::SceneCutLayoutIO
