#include "core/TimesheetSceneResolver.h"

#include <cstdlib>
#include <cmath>
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

bool nearlyEqual(float a, float b)
{
    return std::fabs(a - b) < 0.001f;
}

Timesheet makeTimesheet()
{
    Timesheet timesheet;
    timesheet.totalFrames = 4;

    TimesheetCellTrack a;
    a.cellId = "A";
    a.displayName = "A";
    a.entries.push_back(TimesheetEntry{1, TimesheetEntryType::Drawing, 1, {}});
    a.entries.push_back(TimesheetEntry{2, TimesheetEntryType::Key, 3, {}});
    a.entries.push_back(TimesheetEntry{3, TimesheetEntryType::Null, 0, {}});
    a.entries.push_back(TimesheetEntry{4, TimesheetEntryType::Hold, 0, {}});
    timesheet.tracks.push_back(a);

    TimesheetCellTrack b;
    b.cellId = "B";
    b.displayName = "B";
    b.entries.push_back(TimesheetEntry{1, TimesheetEntryType::Null, 0, {}});
    b.entries.push_back(TimesheetEntry{2, TimesheetEntryType::Drawing, 2, {}});
    b.entries.push_back(TimesheetEntry{3, TimesheetEntryType::Hold, 0, {}});
    timesheet.tracks.push_back(b);

    TimesheetCellTrack c;
    c.cellId = "C";
    c.displayName = "C";
    c.entries.push_back(TimesheetEntry{1, TimesheetEntryType::Drawing, 99, {}});
    timesheet.tracks.push_back(c);

    normalizeTimesheet(timesheet);
    return timesheet;
}

} // namespace

int main()
{
    Cell cellA = makeCell("A", 3);
    cellA.placement.x = 5.0f;
    CellMotionKey motionA;
    motionA.frame = 2;
    motionA.placement.x = 40.0f;
    cellA.motionKeys.push_back(motionA);
    Cell cellB = makeCell("B", 2);
    Cell cellC = makeCell("C", 1);
    Cell hidden = makeCell("A", 3);
    hidden.visible = false;

    const Timesheet timesheet = makeTimesheet();

    const std::vector<TimesheetSceneCellInput> inputs{
        TimesheetSceneCellInput{&cellA, 0, true},
        TimesheetSceneCellInput{&cellB, 1, true},
        TimesheetSceneCellInput{&cellC, 2, true},
    };

    ResolvedTimesheetSceneFrame t1 = resolveTimesheetSceneFrame(timesheet, inputs, 1);
    require(t1.timelineFrame == 1, "T1 frame number");
    require(t1.cells.size() == 1U, "T1 only A visible");
    require(t1.cells[0].cell == &cellA, "T1 A cell pointer");
    require(t1.cells[0].cellIndex == 0, "T1 A cell index");
    require(t1.cells[0].drawingFrameNumber == 1, "T1 A drawing F1");
    require(t1.cells[0].drawingFrameIndex == 0, "T1 A frame index 0");

    ResolvedTimesheetSceneFrame t2 = resolveTimesheetSceneFrame(timesheet, inputs, 2);
    require(t2.cells.size() == 2U, "T2 A and B visible");
    require(t2.cells[0].cell == &cellA && t2.cells[0].drawingFrameIndex == 2, "T2 A F3");
    require(nearlyEqual(t2.cells[0].placement.x, 40.0f), "T2 A motion placement");
    require(t2.cells[1].cell == &cellB && t2.cells[1].drawingFrameIndex == 1, "T2 B F2");

    ResolvedTimesheetSceneFrame t3 = resolveTimesheetSceneFrame(timesheet, inputs, 3);
    require(t3.cells.size() == 1U, "T3 A hidden and B hold visible");
    require(t3.cells[0].cell == &cellB && t3.cells[0].drawingFrameIndex == 1, "T3 B holds F2");

    ResolvedTimesheetSceneFrame t4 = resolveTimesheetSceneFrame(timesheet, inputs, 4);
    require(t4.cells.size() == 1U, "T4 A remains hidden and B holds visible");
    require(t4.cells[0].cell == &cellB && t4.cells[0].drawingFrameIndex == 1, "T4 B still F2");

    const std::vector<TimesheetSceneCellInput> hiddenRequired{
        TimesheetSceneCellInput{&hidden, 10, true},
    };
    require(resolveTimesheetSceneFrame(timesheet, hiddenRequired, 1).cells.empty(),
            "hidden cell skipped when visibility required");

    const std::vector<TimesheetSceneCellInput> hiddenAllowed{
        TimesheetSceneCellInput{&hidden, 10, false},
    };
    require(resolveTimesheetSceneFrame(timesheet, hiddenAllowed, 1).cells.size() == 1U,
            "hidden active/solo cell can be resolved when visibility is not required");

    std::cout << "Timesheet scene resolver self-test passed\n";
    return 0;
}
