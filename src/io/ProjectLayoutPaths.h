#pragma once

// final_spec_v6 Phase 2 Step 2-a.
// Defines path helpers for the future Project -> Scene -> Cut -> Cell folder layout.
// This file intentionally does not change current ProjectIO or CutIO behavior.

#include <filesystem>
#include <string>
#include <string_view>

namespace perapera::ProjectLayoutPaths {

inline std::string sanitizeId(std::string_view value, std::string_view fallback) {
    std::string out;
    out.reserve(value.size());

    for (const char ch : value) {
        const bool isAlpha = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
        const bool isDigit = (ch >= '0' && ch <= '9');
        const bool isSafe = isAlpha || isDigit || ch == '_' || ch == '-';
        out.push_back(isSafe ? ch : '_');
    }

    if (out.empty()) {
        out.assign(fallback.begin(), fallback.end());
    }
    return out;
}

inline std::filesystem::path scenesRoot(const std::filesystem::path& projectRoot) {
    return projectRoot / "scenes";
}

inline std::filesystem::path sceneFolder(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId) {
    return scenesRoot(projectRoot) / sanitizeId(sceneId, "scene_001");
}

inline std::filesystem::path sceneJsonPath(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId) {
    return sceneFolder(projectRoot, sceneId) / "scene.json";
}

inline std::filesystem::path cutsRoot(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId) {
    return sceneFolder(projectRoot, sceneId) / "cuts";
}

inline std::filesystem::path cutFolder(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId) {
    return cutsRoot(projectRoot, sceneId) / sanitizeId(cutId, "cut_001");
}

inline std::filesystem::path cutJsonPath(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId) {
    return cutFolder(projectRoot, sceneId, cutId) / "cut.json";
}

inline std::filesystem::path timesheetJsonPath(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId) {
    return cutFolder(projectRoot, sceneId, cutId) / "timesheet.json";
}

inline std::filesystem::path cellsRoot(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId) {
    return cutFolder(projectRoot, sceneId, cutId) / "cells";
}

inline std::filesystem::path cellFolder(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId,
    std::string_view cellId) {
    return cellsRoot(projectRoot, sceneId, cutId) / sanitizeId(cellId, "cell_001");
}

inline std::filesystem::path cellJsonPath(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId,
    std::string_view cellId) {
    return cellFolder(projectRoot, sceneId, cutId, cellId) / "cell.json";
}

inline std::filesystem::path framesRoot(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId,
    std::string_view cellId) {
    return cellFolder(projectRoot, sceneId, cutId, cellId) / "frames";
}

inline bool createCutFolderSkeleton(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId,
    std::error_code& errorCode) {
    errorCode.clear();
    std::filesystem::create_directories(cellsRoot(projectRoot, sceneId, cutId), errorCode);
    return !errorCode;
}

inline bool createCellFolderSkeleton(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    std::string_view cutId,
    std::string_view cellId,
    std::error_code& errorCode) {
    errorCode.clear();
    std::filesystem::create_directories(framesRoot(projectRoot, sceneId, cutId, cellId), errorCode);
    return !errorCode;
}

} // namespace perapera::ProjectLayoutPaths
