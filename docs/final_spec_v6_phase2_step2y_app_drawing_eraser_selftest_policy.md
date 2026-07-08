# Phase 2 Step 2-y: AppDrawingMode eraser selftest policy

- Added a lightweight selftest for `AppDrawingModeEraser`.
- Covered no-hit preservation, crossing eraser splitting, large short-tap local gap behavior, and empty input stability.
- Kept the test independent from ImGui, SDL windows, and app startup.
- This guards the Step 2-x split before further drawing-mode cleanup.
