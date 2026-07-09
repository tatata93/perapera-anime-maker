# Phase 2 Step 3-f: Cell motion resolver policy

Purpose:

- Start connecting saved `Cell.placement` and `Cell.motionKeys` to the Timesheet resolved display/export data path.
- Keep startup and project loading light.

Implementation:

- Added `resolveCellPlacementAtFrame(...)`.
- The resolver returns base `Cell.placement` before the first key, holds the last key after the final key, supports `hold`, and linearly interpolates `x`, `y`, `scale`, and `rotation`.
- `ResolvedTimesheetSceneCell` now carries resolved `CellPlacement`.
- `TimesheetSceneResolver` fills placement for each resolved visible cell.
- `PngTimesheetExporter` copies the resolved placement into its per-frame export cell data.

Scope:

- This step connects motion data to resolved preview/export data.
- Pixel-level canvas/export transform is not implemented here; that should be a later focused rendering step because it touches renderer math and PNG raster composition.

Verification:

- `perapera_cell_motion_resolver_selftest` builds and passes.
- `perapera_timesheet_scene_resolver_selftest` builds and passes.
- `perapera_png_timesheet_exporter_selftest` builds and passes.
- Debug `perapera_anime_maker` builds and produces `build/bin/perapera_anime_maker.exe`.

Next recommended step:

- Phase 2 Step 3-g: Timesheet UI usability pass, or a focused render transform step if visual Cell motion must be visible before UI work.
