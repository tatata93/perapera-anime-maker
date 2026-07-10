# GitHub progress handoff for ChatGPT

Last updated: 2026-07-09

## Current branch

- Branch: `main`
- Latest local step after this update: `Phase 2 Step 3-h: PNG placement export`
- Main purpose of the latest work: apply CellPlacement x/y/scale to PNG sequence / MP4 pre-export frames so export matches canvas preview.

## Recent completed steps

- Step 2-u: added `CellPanelModel` selftest.
- Step 2-v: split `CanvasRendererSupport` helpers from `CanvasRenderer.cpp`.
- Step 2-w: added `CanvasRendererSupport` selftest.
- Step 2-x: split eraser stroke-splitting helpers from `AppDrawingMode.cpp`.
- Step 2-y: added eraser helper selftest.
- Step 2-z: split onion/eraser overlay drawing helpers from `AppDrawingMode.cpp`.
- Step 2-aa: split drawing-mode timesheet helpers from `AppDrawingMode.cpp`.
- Step 2-ab: added drawing-mode timesheet helper selftest.
- Step 2-ac: split and cleaned preview helper registration state.
- Step 2-ad: split drawing-mode timesheet ViewModel construction helper.
- Step 2-ae: split drawing workspace layout shell.
- Step 2-af: cleaned preview helper residue from `AppDrawingMode.cpp`.
- Step 2-ag: split drawing-mode left/right panels and timeline area into `AppDrawingModePanels.cpp`.
- Step 2-ah: split canvas drawing and stroke finish/cancel helpers into `AppDrawingModeCanvas.cpp`.
- Step 2-ai: split timesheet playback helpers into `AppDrawingModeTimesheetPlayback.cpp`.
- Step 2-aj: added pure helper coverage for timesheet preview/range playback stepping.
- Step 3-a: added optional Cut-owned camera metadata and app save/load bridge.
- Step 3-b: verified Cut camera metadata survives project layout save/load.
- Step 3-c: clarified the spec entry point and added the remaining Phase 2 Step 3-c through Step 3-h roadmap.
- Step 3-d: added Timesheet-aware PNG active-frame, PNG sequence, and MP4 pre-export paths.
- Step 3-e: added resolved cell order helper and connected display/export visible-cell order to `Project.cellOrder`.
- Step 3-f: added Cell motion placement resolver and carried resolved placement through Timesheet scene/export data.
- Step 3-g: applied CellPlacement x/y/scale to canvas preview drawing.
- Step 3-h: applied CellPlacement x/y/scale to PNG export blending.

## Important current state

- `src/render/CanvasRenderer.cpp` is now below the 800-line limit.
- `src/ui/panels/CellPanel.cpp` is below the 800-line limit.
- `src/ui/AppDrawingMode.cpp` is under the 800-line guideline, reduced from about 1917 lines to about 760 lines.
- The app save/load path is already on the new layout IO route, not legacy `ProjectIO::save/load`.
- `DrawingNewLayoutIO` should not be restored.
- Primary spec / AI instruction entry: `docs/perapera_anime_maker_spec_and_ai_work_instruction_v6.md`.
- Historical canonical spec: `docs/final_spec_v6.md`.
- When Timesheet entries exist, export now uses the Timesheet timeline T values. Projects without Timesheet entries use the previous direct frame-index export path.
- `src/io/PngExporter.cpp` is below the 800-line guideline after splitting export mode labels.
- `Project.cellOrder` is the current active Cut order during migration; save writes it as `Cut.cellZOrderKeys` and load restores it.
- True frame-varying z-order keyframes are still a later data-model/UI step.
- `Cell.placement` / `Cell.motionKeys` now resolve into `ResolvedTimesheetSceneCell::placement`.
- Canvas preview now applies placement x/y/scale.
- PNG sequence and MP4 pre-export now apply placement x/y/scale.
- Rotation is still deferred.

## Verification already performed

- `cmake -S . -B .\build -G "Visual Studio 18 2026"` succeeded.
- `perapera_png_timesheet_exporter_selftest` built and passed.
- `perapera_cell_order_resolver_selftest` built and passed.
- `perapera_cell_motion_resolver_selftest` built and passed.
- `perapera_timesheet_scene_resolver_selftest` built and passed after placement coverage was added.
- `perapera_png_timesheet_exporter_selftest` was rerun after PNG placement export and passed.
- Debug app build succeeded.
- `build\bin\perapera_anime_maker.exe` exists.
- Lightweight selftests that were run successfully:
  - `perapera_canvas_renderer_support_selftest`
  - `perapera_app_drawing_mode_eraser_selftest`
  - `perapera_app_drawing_mode_timesheet_selftest`
  - Earlier: `perapera_cell_panel_model_selftest`

## Recommended next work

1. Continue with Phase 2 Step 3-i.
2. Improve vertical Timesheet UI usability after the data/display/export path is protected.
3. Avoid adding startup/project-load scanning or preview-file generation.

## Suggested next candidate

Phase 2 Step 3-i: Timesheet UI usability pass.

Useful commands:

```powershell
git status --short --branch
git log --oneline -12
cmake -S . -B .\build -G "Visual Studio 18 2026"
cmake --build .\build --config Debug --target perapera_anime_maker --parallel 1 -- /m:1 /nodeReuse:false
```

## Phase 2 Step 3-i: Timesheet UI usability
- Status: implemented locally; build verification pending in this Codex turn.
- Files changed: `src/ui/panels/TimesheetPanel.h`, `src/ui/panels/TimesheetPanel.cpp`, `docs/final_spec_v6.md`, `docs/final_spec_v6_phase2_step3i_timesheet_ui_usability_policy.md`, `WORK_LOG.md`, `DECISIONS.md`, `CHATGPT_HANDOFF.md`, `docs/github_progress_for_chatgpt.md`.
- Summary: vertical Timesheet UI gained lightweight T/cell navigation, direct T jump, focus-to-selected-row, row-height control, and centralized selection handling.
- Startup/load policy: unchanged; no preview generation or project-load scanning was added.
- Next recommended: Phase 2 Step 3-j closeout audit.
Verification for Phase 2 Step 3-i:
- CMake configure: passed.
- Debug app build target `perapera_anime_maker`: passed.
- `perapera_timesheet_panel_bridge_selftest`: built and passed.
- `build/bin/perapera_anime_maker.exe`: exists.
## Phase 2 Step 3-j: closeout audit
- Status: implemented locally; build/selftest verification pending in this Codex turn.
- Summary: Phase 2 active source audit found no legacy AppProjectIO ProjectIO save/load usage, no DrawingNewLayoutIO revival, and no Scene Plate implementation revival.
- Fixed: one stale App UI message now follows normal Cell + Timesheet management instead of Scene Plate wording.
- Line limit: all `src/` files remain under 800 lines. Near-limit files are documented for split-before-bulk future work.
- Spec: Phase 2 is marked closed after Step 3-j; next recommended focus is Phase 3 preparation for shooting/compositing.
Verification for Phase 2 Step 3-j:
- CMake configure: passed.
- Debug app build target `perapera_anime_maker`: passed.
- Selftests passed: `perapera_project_layout_load_entry_selftest`, `perapera_project_layout_save_entry_selftest`, `perapera_project_layout_read_entry_selftest`, `perapera_app_project_io_support_selftest`, `perapera_cut_cell_timesheet_bridge_selftest`, `perapera_timesheet_panel_bridge_selftest`, `perapera_timesheet_scene_resolver_selftest`, `perapera_cell_order_resolver_selftest`, `perapera_cell_motion_resolver_selftest`, `perapera_png_timesheet_exporter_selftest`.
- `build/bin/perapera_anime_maker.exe`: exists.
## Phase 3 Step 3-a: Camera resolver foundation
- Status: implemented locally; build/selftest verification pending in this Codex turn.
- Summary: added a pure camera resolver so shooting preview/export/UI can share one timeline-T camera result.
- Startup/load policy: unchanged; no preview generation or project-load scanning was added.
- Next recommended: Phase 3 Step 3-b shooting-frame data contract combining Timesheet scene output with resolved camera output.
Verification for Phase 3 Step 3-a:
- CMake configure: passed.
- Debug app build target `perapera_anime_maker`: passed.
- `perapera_camera_resolver_selftest`: built and passed.
- `build/bin/perapera_anime_maker.exe`: exists.