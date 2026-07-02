#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "core/Cell.h"
#include "core/Frame.h"
#include "io/CellLayoutIO.h"
#include "io/ProjectLayoutPaths.h"

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

perapera::Frame makeFrame(const std::string& name, int durationFrames) {
    perapera::Frame frame;
    frame.name = name;
    frame.durationFrames = durationFrames;
    return frame;
}

perapera::Cell makeCell(const std::string& id,
                        const std::string& name,
                        const std::string& category,
                        int frameCount) {
    perapera::Cell cell;
    cell.id = id;
    cell.name = name;
    cell.category = category;
    cell.widthPx = 1920;
    cell.heightPx = 1080;

    for (int index = 0; index < frameCount; ++index) {
        cell.frames.push_back(makeFrame("F" + std::to_string(index + 1), index + 1));
    }
    return cell;
}

} // namespace

int main() {
    const std::filesystem::path root = std::filesystem::temp_directory_path() /
                                      "perapera_cell_layout_io_selftest";
    std::error_code error;
    std::filesystem::remove_all(root, error);

    const std::vector<perapera::Cell> cells = {
        makeCell("cell_A", "Character A", "character", 2),
        makeCell("cell_BG", "Background", "background", 1),
        makeCell("cell_BOOK", "Book", "book", 0),
    };

    perapera::CellLayoutSaveOptions options;
    options.sceneId = "scene_001";
    options.cutId = "cut_001";

    std::vector<perapera::CellLayoutSaveSummary> summaries;
    if (!require(perapera::saveCellsLayoutMinimal(root, cells, options, &summaries),
                 "saveCellsLayoutMinimal should succeed")) {
        return 1;
    }

    if (!require(summaries.size() == cells.size(), "summary count should match cell count")) {
        return 1;
    }

    if (!require(std::filesystem::exists(perapera::cellJsonPath(root, "scene_001", "cut_001", "cell_A")),
                 "cell_A/cell.json should exist")) {
        return 1;
    }
    if (!require(std::filesystem::exists(perapera::frameJsonPath(root, "scene_001", "cut_001", "cell_A", 0)),
                 "cell_A frame_001/frame.json should exist")) {
        return 1;
    }
    if (!require(std::filesystem::exists(perapera::frameJsonPath(root, "scene_001", "cut_001", "cell_A", 1)),
                 "cell_A frame_002/frame.json should exist")) {
        return 1;
    }
    if (!require(std::filesystem::exists(perapera::cellJsonPath(root, "scene_001", "cut_001", "cell_BG")),
                 "cell_BG/cell.json should exist")) {
        return 1;
    }
    if (!require(std::filesystem::is_directory(perapera::framesDirectory(root, "scene_001", "cut_001", "cell_BOOK")),
                 "empty BOOK cell should still have frames directory")) {
        return 1;
    }

    std::filesystem::remove_all(root, error);
    std::cout << "perapera_cell_layout_io_selftest passed\n";
    return 0;
}
