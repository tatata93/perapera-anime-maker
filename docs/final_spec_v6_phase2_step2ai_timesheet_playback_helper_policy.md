# Phase 2 Step 2-ai: Timesheet playback helper split policy

- Move the drawing-mode timesheet preview playback helpers from local lambdas into App methods in `AppDrawingModeTimesheetPlayback.cpp`.
- Keep selected-T preview movement, range playback, ping-pong behavior, edit-target synchronization, and renderer invalidation unchanged.
- Do not add startup warmup, project-load scanning, or preview-file generation.
- This step is intended to bring `AppDrawingMode.cpp` under the 800-line guideline without changing user-facing behavior.
