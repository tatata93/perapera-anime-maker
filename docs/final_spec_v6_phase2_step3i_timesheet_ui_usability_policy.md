# Phase 2 Step 3-i: Timesheet UI Usability Policy

Phase 2 Step 3-i improves the vertical Timesheet window without changing project save/load format or generating preview files during startup/load.

Implemented policy:

- Keep `TimesheetPanel` independent from `CellPanel`.
- Keep Timesheet editing connected through `TimesheetPanelState` and `TimesheetPanelBridge`.
- Add lightweight navigation helpers for frame and cell selection.
- Add direct T jump, focus-to-selected-row, and row-height adjustment controls.
- Keep selected-frame scrolling lazy and UI-only.
- Do not add image loading, preview file generation, or project-load scanning.

Deferred:

- Keyboard-first bulk entry workflow.
- Drag-and-drop timing edits.
- Camera columns inside the Timesheet grid.
- Printing / standalone Timesheet view.
