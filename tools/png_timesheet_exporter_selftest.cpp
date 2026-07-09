#include "io/PngTimesheetExporter.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace perapera;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

Cell makeCell(const std::string& id, int frameCount)
{
    Cell cell;
    cell.id = id;
    cell.name = id;
    cell.visible = true;
    cell.opacity = 1.0f;
    cell.frames.resize(static_cast<std::size_t>(frameCount));
    return cell;
}

Timesheet makeTimesheet()
{
    Timesheet timesheet;
    timesheet.totalFrames = 3;

    TimesheetCellTrack a;
    a.cellId = "A";
    a.displayName = "A";
    a.entries.push_back(TimesheetEntry{1, TimesheetEntryType::Drawing, 1, {}});
    a.entries.push_back(TimesheetEntry{2, TimesheetEntryType::Drawing, 2, {}});
    a.entries.push_back(TimesheetEntry{3, TimesheetEntryType::Null, 0, {}});
    timesheet.tracks.push_back(a);

    TimesheetCellTrack b;
    b.cellId = "B";
    b.displayName = "B";
    b.entries.push_back(TimesheetEntry{2, TimesheetEntryType::Drawing, 1, {}});
    timesheet.tracks.push_back(b);

    normalizeTimesheet(timesheet);
    return timesheet;
}

bool existsWithBytes(const std::filesystem::path& path)
{
    std::error_code errorCode;
    return std::filesystem::exists(path, errorCode) &&
        std::filesystem::file_size(path, errorCode) > 0U &&
        !errorCode;
}

} // namespace

int main()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "perapera_png_timesheet_exporter_selftest";
    std::error_code errorCode;
    std::filesystem::remove_all(root, errorCode);

    Cell cellA = makeCell("A", 2);
    Cell cellB = makeCell("B", 1);
    const Timesheet timesheet = makeTimesheet();
    const std::vector<TimesheetSceneCellInput> inputs{
        TimesheetSceneCellInput{&cellA, 0, true},
        TimesheetSceneCellInput{&cellB, 1, true},
    };

    std::string error;
    require(PngTimesheetExporter::exportFrame(timesheet,
                                              inputs,
                                              2,
                                              root / "active_t2.png",
                                              8,
                                              8,
                                              ExportMode::Composite,
                                              &error),
            "active timesheet frame export should succeed");
    require(existsWithBytes(root / "active_t2.png"), "active_t2.png exists");

    require(PngTimesheetExporter::exportFrameSequence(timesheet,
                                                      inputs,
                                                      root / "sequence",
                                                      8,
                                                      8,
                                                      ExportMode::Composite,
                                                      &error),
            "timesheet sequence export should succeed");
    require(existsWithBytes(root / "sequence" / "frame_001.png"), "frame_001.png exists");
    require(existsWithBytes(root / "sequence" / "frame_002.png"), "frame_002.png exists");
    require(existsWithBytes(root / "sequence" / "frame_003.png"), "blank frame_003.png exists");
    require(!std::filesystem::exists(root / "sequence" / "frame_004.png"), "sequence stops at timesheet totalFrames");

    std::filesystem::remove_all(root, errorCode);
    std::cout << "perapera_png_timesheet_exporter_selftest passed\n";
    return 0;
}
