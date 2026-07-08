# Phase 2 Step 2-ab: AppDrawingMode timesheet selftest policy

- Added a lightweight selftest for `AppDrawingModeTimesheet`.
- Covered entry counting, selected panel entry lookup, project cell lookup, missing drawing-frame creation, and playback-order navigation.
- Kept the test independent from ImGui drawing, SDL windows, and app startup.
- This guards the Step 2-aa split before more drawing-mode cleanup.
