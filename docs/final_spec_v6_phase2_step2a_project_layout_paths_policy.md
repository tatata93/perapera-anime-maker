# final_spec_v6 Phase 2 Step 2-a: Project Layout Paths

## Position

- Top-level spec: `final_spec_v6.md`
- Phase: Phase 2, file structure revision and cell model organization
- Step: Step 2-a
- Scope: define and test future project folder paths only

## Decision

This step does not replace the current project save/load implementation.
It only adds path helpers for the future folder layout:

```text
scenes/scene_001/scene.json
scenes/scene_001/cuts/cut_001/cut.json
scenes/scene_001/cuts/cut_001/timesheet.json
scenes/scene_001/cuts/cut_001/cells/cell_BG/cell.json
scenes/scene_001/cuts/cut_001/cells/cell_BG/frames/
```

Timesheet remains physically separable as `timesheet.json` so it can later support standalone viewing and printing.

## Non-goals

- Do not change ProjectIO yet.
- Do not change CutIO yet.
- Do not move existing Project.cells yet.
- Do not change UI.
- Do not reintroduce Scene Plate or scene-panel code.

## File-size policy

All added files are under 800 lines. Existing large UI files are not modified in this step.
