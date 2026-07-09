# Phase 2 Step 3-d: Timesheet export propagation policy

Purpose:

- Connect Cut-owned `workingTimesheet_` to PNG/MP4 export when Timesheet entries exist.
- Keep the old direct frame-index export path for projects without Timesheet entries.
- Avoid startup or project-load preview generation.

Implementation:

- Added `PngTimesheetExporter` as a small exporter wrapper that resolves each timeline T through `TimesheetSceneResolver`.
- PNG active-frame export uses the selected Timesheet T when Timesheet entries exist.
- PNG sequence and MP4 pre-export use `timesheet.totalFrames` and output one PNG per timeline T.
- Blank Timesheet frames are exported as blank PNG frames instead of failing.
- Split PNG export mode labels to `PngExporterModes.cpp`, reducing `PngExporter.cpp` below the 800-line guideline.

Verification:

- `perapera_png_timesheet_exporter_selftest` builds and passes.
- Debug `perapera_anime_maker` builds and produces `build/bin/perapera_anime_maker.exe`.

Next recommended step:

- Phase 2 Step 3-e: connect `cellZOrderKeys` to resolved display/export ordering before expanding UI.
