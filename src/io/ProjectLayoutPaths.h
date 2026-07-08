#pragma once

// final_spec_v6 Phase 2 Step 2 path helpers.
// Keep this header small and header-only so IO steps can share the new layout
// without pulling in save/load entry behavior.

#include <filesystem>
#include <string>

namespace perapera {

inline std::string normalizeLayoutId(std::string id, const std::string& fallback) {
    if (id.empty()) {
        return fallback;
    }

    for (char& ch : id) {
        const bool alpha = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
        const bool digit = (ch >= '0' && ch <= '9');
        if (!alpha && !digit && ch != '_' && ch != '-') {
            ch = '_';
        }
    }
    return id.empty() ? fallback : id;
}

inline std::string defaultSceneId() {
    return "scene_001";
}

inline std::string defaultCutId() {
    return "cut_001";
}

inline std::string frameDirectoryName(int zeroBasedFrameIndex) {
    const int number = zeroBasedFrameIndex + 1;
    const int hundreds = (number / 100) % 10;
    const int tens = (number / 10) % 10;
    const int ones = number % 10;

    std::string name = "frame_";
    name.push_back(static_cast<char>('0' + hundreds));
    name.push_back(static_cast<char>('0' + tens));
    name.push_back(static_cast<char>('0' + ones));
    return name;
}

inline std::filesystem::path scenesDirectory(const std::filesystem::path& projectRoot) {
    return projectRoot / "scenes";
}

inline std::filesystem::path sceneDirectory(const std::filesystem::path& projectRoot,
                                            const std::string& sceneId) {
    return scenesDirectory(projectRoot) / normalizeLayoutId(sceneId, defaultSceneId());
}

inline std::filesystem::path sceneJsonPath(const std::filesystem::path& projectRoot,
                                           const std::string& sceneId) {
    return sceneDirectory(projectRoot, sceneId) / "scene.json";
}

inline std::filesystem::path cutsDirectory(const std::filesystem::path& projectRoot,
                                           const std::string& sceneId) {
    return sceneDirectory(projectRoot, sceneId) / "cuts";
}

inline std::filesystem::path cutDirectory(const std::filesystem::path& projectRoot,
                                          const std::string& sceneId,
                                          const std::string& cutId) {
    return cutsDirectory(projectRoot, sceneId) / normalizeLayoutId(cutId, defaultCutId());
}

inline std::filesystem::path cutJsonPath(const std::filesystem::path& projectRoot,
                                         const std::string& sceneId,
                                         const std::string& cutId) {
    return cutDirectory(projectRoot, sceneId, cutId) / "cut.json";
}

inline std::filesystem::path cutTimesheetJsonPath(const std::filesystem::path& projectRoot,
                                                  const std::string& sceneId,
                                                  const std::string& cutId) {
    return cutDirectory(projectRoot, sceneId, cutId) / "timesheet.json";
}

inline std::filesystem::path cellsDirectory(const std::filesystem::path& projectRoot,
                                            const std::string& sceneId,
                                            const std::string& cutId) {
    return cutDirectory(projectRoot, sceneId, cutId) / "cells";
}

inline std::filesystem::path cellDirectory(const std::filesystem::path& projectRoot,
                                           const std::string& sceneId,
                                           const std::string& cutId,
                                           const std::string& cellId) {
    return cellsDirectory(projectRoot, sceneId, cutId) / normalizeLayoutId(cellId, "cell_001");
}

inline std::filesystem::path cellJsonPath(const std::filesystem::path& projectRoot,
                                          const std::string& sceneId,
                                          const std::string& cutId,
                                          const std::string& cellId) {
    return cellDirectory(projectRoot, sceneId, cutId, cellId) / "cell.json";
}

inline std::filesystem::path framesDirectory(const std::filesystem::path& projectRoot,
                                             const std::string& sceneId,
                                             const std::string& cutId,
                                             const std::string& cellId) {
    return cellDirectory(projectRoot, sceneId, cutId, cellId) / "frames";
}

inline std::filesystem::path frameDirectory(const std::filesystem::path& projectRoot,
                                            const std::string& sceneId,
                                            const std::string& cutId,
                                            const std::string& cellId,
                                            int zeroBasedFrameIndex) {
    return framesDirectory(projectRoot, sceneId, cutId, cellId) /
           frameDirectoryName(zeroBasedFrameIndex);
}

inline std::filesystem::path frameJsonPath(const std::filesystem::path& projectRoot,
                                           const std::string& sceneId,
                                           const std::string& cutId,
                                           const std::string& cellId,
                                           int zeroBasedFrameIndex) {
    return frameDirectory(projectRoot, sceneId, cutId, cellId, zeroBasedFrameIndex) / "frame.json";
}

} // namespace perapera
