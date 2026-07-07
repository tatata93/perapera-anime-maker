# final_spec_v6 Phase 2 Step 1-b: Project active Cut bridge policy

## Position in the master spec

- Master spec: `final_spec_v6.md`
- Phase: Phase 2, file structure revision and Cell concept organization
- Step: Step 1-b, bridge existing `Project.cells` as the current active Cut cells

## Decision

The project is moving toward `Project -> Scene -> Cut -> Cell`, but this step does not change the save format yet.
Existing `Project.cells` and `Project.cellOrder` are temporarily treated as the cells and order of the active Cut.

This is a migration bridge only. It keeps the current UI and project loading stable while later steps move ownership into real Scene/Cut structures.

## Rules

- Do not move existing cells into a new folder structure in this step.
- Do not change `ProjectIO.cpp` in this step.
- Do not add a scene-management panel.
- Do not revive Scene Plate or ScenePlateImageCache.
- Background, layout, book, effect, and reference items remain normal Cells.
- Timesheet remains conceptually Cut-owned, but may be stored separately for future standalone display and printing.

## Added API

`src/core/ProjectActiveCutBridge.h` provides:

- `activeCutCells(Project&)`
- `activeCutCells(const Project&)`
- `activeCutContainsCell(...)`
- `ensureActiveCutCell(...)`
- `ensureActiveCutCellOrder(...)`

These helpers provide a clear migration seam so UI and selftests can gradually stop depending directly on `Project.cells`.
