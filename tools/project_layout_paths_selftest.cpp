#include "io/ProjectLayoutPaths.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }
    return true;
}

std::string generic(const std::filesystem::path& path) {
    return path.generic_string();
}

} // namespace

int main() {
    namespace paths = perapera::ProjectLayoutPaths;

    const std::filesystem::path root = "project_root";
    bool ok = true;

    ok &= expect(generic(paths::scenesRoot(root)) == "project_root/scenes", "scenes root path");
    ok &= expect(generic(paths::sceneFolder(root, "scene_001")) == "project_root/scenes/scene_001", "scene folder path");
    ok &= expect(generic(paths::sceneJsonPath(root, "scene_001")) == "project_root/scenes/scene_001/scene.json", "scene json path");
    ok &= expect(generic(paths::cutsRoot(root, "scene_001")) == "project_root/scenes/scene_001/cuts", "cuts root path");
    ok &= expect(generic(paths::cutFolder(root, "scene_001", "cut_001")) == "project_root/scenes/scene_001/cuts/cut_001", "cut folder path");
    ok &= expect(generic(paths::cutJsonPath(root, "scene_001", "cut_001")) == "project_root/scenes/scene_001/cuts/cut_001/cut.json", "cut json path");
    ok &= expect(generic(paths::timesheetJsonPath(root, "scene_001", "cut_001")) == "project_root/scenes/scene_001/cuts/cut_001/timesheet.json", "timesheet json path");
    ok &= expect(generic(paths::cellsRoot(root, "scene_001", "cut_001")) == "project_root/scenes/scene_001/cuts/cut_001/cells", "cells root path");
    ok &= expect(generic(paths::cellFolder(root, "scene_001", "cut_001", "cell_BG")) == "project_root/scenes/scene_001/cuts/cut_001/cells/cell_BG", "cell folder path");
    ok &= expect(generic(paths::cellJsonPath(root, "scene_001", "cut_001", "cell_BG")) == "project_root/scenes/scene_001/cuts/cut_001/cells/cell_BG/cell.json", "cell json path");
    ok &= expect(generic(paths::framesRoot(root, "scene_001", "cut_001", "cell_BG")) == "project_root/scenes/scene_001/cuts/cut_001/cells/cell_BG/frames", "frames root path");

    ok &= expect(paths::sanitizeId("scene 001", "fallback") == "scene_001", "sanitize space");
    ok &= expect(paths::sanitizeId("", "fallback") == "fallback", "sanitize empty fallback");
    ok &= expect(paths::sanitizeId("cell/A", "fallback") == "cell_A", "sanitize slash");

    const auto tempRoot = std::filesystem::temp_directory_path() / "perapera_project_layout_paths_selftest";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
    ec.clear();

    ok &= expect(paths::createCutFolderSkeleton(tempRoot, "scene_001", "cut_001", ec), "create cut skeleton");
    ok &= expect(std::filesystem::is_directory(paths::cellsRoot(tempRoot, "scene_001", "cut_001")), "cells dir exists");

    ok &= expect(paths::createCellFolderSkeleton(tempRoot, "scene_001", "cut_001", "cell_BG", ec), "create cell skeleton");
    ok &= expect(std::filesystem::is_directory(paths::framesRoot(tempRoot, "scene_001", "cut_001", "cell_BG")), "frames dir exists");

    std::filesystem::remove_all(tempRoot, ec);

    if (!ok) {
        return 1;
    }

    std::cout << "perapera_project_layout_paths_selftest passed\n";
    return 0;
}
