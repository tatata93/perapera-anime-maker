# Phase 2 Step 2-aa: AppDrawingMode timesheet helper split policy

- Split drawing-mode timesheet helper logic into `AppDrawingModeTimesheet`.
- Moved entry counting, panel entry selection, project cell lookup, missing drawing-frame creation, and timesheet playback-order helpers.
- Kept App state, UI rendering flow, and mode transitions in `AppDrawingMode.cpp`.
- This reduces the large drawing-mode file while keeping timesheet behavior connected to the current UI.
