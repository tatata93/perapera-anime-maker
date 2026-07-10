# Phase 2 Step 3-j: Closeout Audit

Phase 2 Step 3-j closes the current Project -> Scene -> Cut -> Cell structure pass.

Audit result:

- Active app save/load code no longer calls `ProjectIO::save` or `ProjectIO::load` directly from `AppProjectIO.cpp`.
- Active app save/load uses `ProjectLayoutSaveEntry` / `ProjectLayoutLoadEntry`.
- `DrawingNewLayoutIO` is not present in active source files or CMake targets.
- Scene Plate / separate background panel code is not present in active source files or CMake targets.
- One stale app UI message still described Scene Plate as a separate path; it was corrected to the current normal Cell + Timesheet policy.
- Source files under `src/` remain under the 800-line guideline at this closeout.
- Near-limit files should be split before future feature bulk:
  - `src/io/PngExporter.cpp`
  - `src/render/CanvasRenderer.cpp`
  - `src/ui/panels/CellPanel.cpp`
  - `src/ui/AppDrawingMode.cpp`

Verification:

- CMake configure must pass.
- Debug `perapera_anime_maker` build must pass.
- Relevant Phase 2 selftests should pass:
  - `perapera_project_layout_load_entry_selftest`
  - `perapera_project_layout_save_entry_selftest`
  - `perapera_project_layout_read_entry_selftest`
  - `perapera_app_project_io_support_selftest`
  - `perapera_cut_cell_timesheet_bridge_selftest`
  - `perapera_timesheet_panel_bridge_selftest`
  - `perapera_timesheet_scene_resolver_selftest`
  - `perapera_cell_order_resolver_selftest`
  - `perapera_cell_motion_resolver_selftest`
  - `perapera_png_timesheet_exporter_selftest`

Next recommended work:

- Start a Phase 3 preparation audit for the shooting/compositing path, while keeping startup and project loading lightweight.
- If Phase 2.5 previz/3D is chosen first, decide whether OpenGL/GLAD/GLM setup should be introduced before or after Phase 3 compositing foundations.
