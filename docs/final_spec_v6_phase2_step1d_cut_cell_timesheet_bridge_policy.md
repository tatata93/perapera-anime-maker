# final_spec_v6 Phase 2 Step 1-d: Cut Cell Timesheet bridge policy

## Position

- Master spec: `final_spec_v6.md`
- Phase: Phase 2, file structure revision and Cell concept organization
- Step: Step 1-d, confirm the relationship between active Cut cells and the Cut-owned Timesheet

## Work tree

```text
final_spec_v6.md
└─ Phase 2: file structure revision and Cell concept organization
   ├─ Phase 2-pre: normal Cell + Timesheet path cleanup
   ├─ Phase 2 Step 1: minimal Project -> Scene -> Cut -> Cell introduction
   │  ├─ Step 1-a: Scene / active Cut minimal model
   │  ├─ Step 1-b: Project.cells as active Cut bridge
   │  ├─ Step 1-c: Project / Scene / Cut UI status, currently held back until exe generation is stable
   │  └─ Step 1-d: Cut Cell Timesheet bridge, this step
   ├─ Phase 2 Step 2: scene/cut/cell save folder migration
   ├─ Phase 2 Step 3: Cut-level Timesheet / Cell / Camera connection
   └─ Phase 3-pre: simple shooting/cell placement window
```

## Decision

During migration, existing `Project.cells` and `Project.cellOrder` are the active Cut cells through `ProjectActiveCutBridge`.
The Timesheet is conceptually owned by the active Cut, but may remain physically stored as `timesheet.json` for future standalone display and printing.

All normal Cell categories are eligible for Timesheet tracks:

- `character`
- `background`
- `layout`
- `book`
- `effect`
- `reference`

No category should be routed through Scene Plate or a separate scene panel.

## Added implementation

- `src/core/CutCellTimesheetBridge.h`
  - `isTimelineManagedCellCategory(...)`
  - `activeCutCellIdsForTimesheet(...)`
  - `ensureTimesheetTracksForActiveCutCells(...)`
- `tools/cut_cell_timesheet_bridge_selftest.cpp`
  - Confirms that all 6 normal categories receive Timesheet tracks.
  - Confirms existing BG tracks are not duplicated.
  - Confirms existing entries are preserved.

## Not in this step

- No Project save format change.
- No `Project.h` scenes field yet.
- No UI panel.
- No Scene Plate revival.
- No shooting/cell placement UI yet.
