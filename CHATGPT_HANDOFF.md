# ChatGPT handoff

Current branch/state:

- Work is on `main`.
- GitHub should have only `main`.
- Latest completed step: final_spec_v6 Phase 2 Step 2-g.

What was just done:

- Added Step 2-e save entry earlier:
  - `src/io/ProjectLayoutSaveEntry.h`
  - `src/io/ProjectLayoutSaveEntry.cpp`
  - `tools/project_layout_save_entry_selftest.cpp`
- Connected Step 2-f app save:
  - `src/ui/AppProjectIO.cpp`
  - `App::saveProject()` now writes both the legacy save and the new `scenes/scene_001/cuts/cut_001/` layout.
  - `App::saveLoadRoundTripCheck()` also writes the new layout, then still verifies through the existing legacy load path.
- Added Step 2-g read/inspection entry:
  - `src/io/ProjectLayoutReadEntry.h`
  - `src/io/ProjectLayoutReadEntry.cpp`
  - `tools/project_layout_read_entry_selftest.cpp`
  - It validates `scene.json`, `cut.json`, `timesheet.json`, `cell.json`, `frame.json`, and layer JSON files.

Important direction:

- Do not revive Scene Plate, ScenePlateImageCache, or the old scene panel.
- Background, layout, BOOK, effect, and reference should remain normal Cells timed by the Timesheet.
- Backward compatibility with old project files is not required during development.
- Keep UI changes small until the new save/load path is stable.

Verification already done:

- `cmake --build .\build --config Debug --target perapera_anime_maker --parallel 1`
- All `build/bin/*selftest.exe` passed: 17 selftests.

Recommended next task:

- Phase 2 Step 2-h: turn the new-layout inspector into a real app load path.
- Keep the legacy load path available until the new layout can reconstruct enough Project data safely.

User confirmation that would help:

- Launch `build/bin/perapera_anime_maker.exe`.
- Use Save once.
- Confirm the project folder now contains `scenes/scene_001/cuts/cut_001/` with `cut.json`, `timesheet.json`, and `cells/`.

---

## Latest handoff: Phase 2 Step 2-l

Current branch/state:

- Work is on `main`.
- Goal: connect `src/ui/AppProjectIO.cpp` directly to the new layout save/load entries.

Latest intended changes:

- `AppProjectIO.cpp` should not include or call legacy `ProjectIO::save` / `ProjectIO::load`.
- App save should use `ProjectLayoutSaveEntry`.
- App load and explicit round-trip check should use `ProjectLayoutLoadEntry` / `loadProjectNewLayoutMinimal`.
- Loaded `cut.timesheet` should be reflected back to `workingTimesheet_`.
- `DrawingNewLayoutIO` must stay removed.
- Do not intentionally delete `ProjectIO.*` in this step; deletion policy remains Step 2-m.

Files touched in this step:

- `src/ui/AppProjectIO.cpp`
- `CMakeLists.txt`
- `docs/final_spec_v6_phase2_step2l_app_projectio_direct_fix_policy.md`
- `WORK_LOG.md`
- `DECISIONS.md`

Next checks:

- Confirm `AppProjectIO.cpp` no longer contains `ProjectIO::save`, `ProjectIO::load`, or `DrawingNewLayoutIO`.
- Configure and build Debug.
- Confirm `build/bin/perapera_anime_maker.exe` exists.

---

## Latest handoff: Phase 2 Step 2-m closeout

Current state:

- `src/io/ProjectIO.h` and `src/io/ProjectIO.cpp` are absent.
- `CMakeLists.txt` no longer lists `src/io/ProjectIO.cpp`.
- Active app save/load uses `ProjectLayoutSaveEntry` and `ProjectLayoutLoadEntry`.
- `DrawingNewLayoutIO` remains removed.
- Current source comments were cleaned so the active source no longer refers to the old route by name.
- `docs/final_spec_v6_phase2_step2m_projectio_usage_audit.md` was updated to the current state.

Recommended next task:

- Improve new-layout load fidelity. In particular, inspect what `ProjectLayoutLoadEntry.cpp` restores for strokes, fill strokes, layer metadata, canvas/output settings, and timesheet assignment, then add focused selftests before widening app behavior.

---

## Latest handoff: Phase 2 Step 2-n load fidelity

Current state:

- `ProjectLayoutLoadEntry.cpp` now restores saved brush engine, stroke opacity/style fields, Fill bitmap data, and layer type.
- Loaded `Project.cells` / `Project.cellOrder` now follow `Cut.cellZOrderKeys`; unlisted cells are appended afterward.
- Loaded scene/cut metadata now fills minimal `Project` name, timeline total frames, and output FPS.
- `perapera_project_layout_load_entry_selftest` verifies brush settings, Fill bitmap, layer type, FPS, total frames, and cut cell order.

Next recommended task:

- Continue load-fidelity work by checking fields that still are not saved or loaded, especially `Cell.motionKeys`, canvas/output width and height, camera/audio, and any layer/frame metadata needed by the UI.

---

## Latest handoff: Phase 2 Step 2-o project metadata and motion keys

Current state:

- App save now uses the full-`Project` new-layout save overload.
- New-layout save writes root `project.json` for canvas, output, audio, camera, and scene order metadata.
- New-layout load reads `project.json` when present.
- `Cell.motionKeys` are written to `cell.json` and restored on load.
- The project layout load selftest verifies project metadata and cell motion keys.

Next recommended task:

- Inspect remaining unsaved state, especially UI selection/app-state coupling, project/cut multi-scene expansion points, and any renderer cache invalidation needed after loading metadata changes.

---

## Latest handoff: Phase 2 Step 2-p round-trip signature

Current state:

- `appio::projectSignature()` now covers project metadata, cell order, cell motion keys, layer metadata, stroke style fields, brush engine, point pressure, and Fill bitmap samples.
- Fill bitmap hashing remains lightweight: size plus first/middle/last samples, not a full payload scan.
- Manual save/load check should now catch more new-layout field loss without making normal saves heavy.

Next recommended task:

- Add focused coverage around app-level load/save selection behavior or inspect remaining large files for safe split candidates.

---

## Latest handoff: Phase 2 Step 2-q lightweight inspection

Current state:

- `ProjectLayoutReadEntry` avoids full layer JSON parsing in the normal case.
- Layer inspection checks the schema from a small prefix first, so future project lists/startup checks do not read full stroke/Fill bitmap payloads.
- Full parsing remains as a fallback for unusual layer files.

Next recommended task:

- Inspect remaining large UI/render files and split the least risky helpers, or add app-level save/load selection coverage.

---

## Latest handoff: Phase 2 Step 2-r project signature selftest

Current state:

- Added `tools/app_project_io_support_selftest.cpp`.
- Added CMake target `perapera_app_project_io_support_selftest`.
- The selftest verifies `projectSignature()` detects metadata, motion key, Fill bitmap sample, and point pressure changes.
- It also checks basic selection validation behavior.

Next recommended task:

- Run full Debug build after this commit, then consider a low-risk split of an over-800-line file.

---

## Latest handoff: Phase 2 Step 2-s pending doc cleanup

Current state:

- The previously untracked Step 2-k applyfix policy document is now intended to be tracked.
- This removes the recurring untracked doc from `git status` so future work starts cleanly.

Next recommended task:

- Continue with a low-risk large-file split only after reading the target file carefully.

---

## Latest handoff: Phase 2 Step 2-t CellPanel split

Current state:

- Added `src/ui/panels/CellPanelModel.h/.cpp`.
- Moved cell ID, cell order, display label, layer count, duplicate naming, and layer-ID maintenance helpers out of `CellPanel.cpp`.
- `CellPanel.cpp` now keeps ImGui drawing and display-mode state and is below 800 lines.
- Debug app build succeeded after the split.

Next recommended task:

- Consider the next low-risk large-file split, likely `CanvasRenderer.cpp`, but only after isolating pure helper logic.

---

## Latest handoff: Phase 2 Step 2-u CellPanel model selftest

Current state:

- Added `tools/cell_panel_model_selftest.cpp`.
- Added CMake target `perapera_cell_panel_model_selftest`.
- The selftest covers cell order/z-order repair, unique cell IDs, display labels, duplicate names, and cell-scoped layer ID repair.
- The test is intentionally independent from ImGui and renderer code.

Next recommended task:

- Inspect `CanvasRenderer.cpp` for pure helper logic that can be split safely, or continue adding lightweight selftests around hot-path cache behavior before editing renderer internals.

---

## Latest handoff: Phase 2 Step 2-v CanvasRenderer support split

Current state:

- Added `src/render/CanvasRendererSupport.h/.cpp`.
- Moved pure helper logic for display opacity, layer rank, cache IDs, lightweight revisions, hash helpers, alpha conversion, and stroke baking.
- `CanvasRenderer.cpp` still owns renderer state, bitmap caches, progressive rebuild, draw flow, and cache pruning.
- `CanvasRenderer.cpp` is now within the 800-line limit.
- Debug app build succeeded after the split.

Next recommended task:

- Add a small focused selftest for renderer support helper invariants, or inspect the next large file `src/ui/AppDrawingMode.cpp` before any split.

---

## Latest handoff: Phase 2 Step 2-w CanvasRenderer support selftest

Current state:

- Added `tools/canvas_renderer_support_selftest.cpp`.
- Added CMake target `perapera_canvas_renderer_support_selftest`.
- The selftest covers display opacity caps, layer rank, frame/layer cache IDs, lightweight revision values, and Paint append-order safety.
- The test does not start the app or create a renderer window.

Next recommended task:

- Inspect `src/ui/AppDrawingMode.cpp` for a safe split candidate, or add focused tests around app drawing-mode state transitions before editing the large file.

---

## Latest handoff: Phase 2 Step 2-x AppDrawingMode eraser split

Current state:

- Added `src/ui/AppDrawingModeEraser.h/.cpp`.
- Moved pure eraser stroke-splitting helpers out of `AppDrawingMode.cpp`.
- `AppDrawingMode.cpp` still owns onion-skin drawing, eraser preview drawing, App state updates, and renderer dirty notifications.
- Debug app build succeeded after the split.

Next recommended task:

- Add a small selftest for `AppDrawingModeEraser`, then continue splitting AppDrawingMode by similarly closed helper groups.

---

## Latest handoff: Phase 2 Step 2-y AppDrawingMode eraser selftest

Current state:

- Added `tools/app_drawing_mode_eraser_selftest.cpp`.
- Added CMake target `perapera_app_drawing_mode_eraser_selftest`.
- The selftest covers no-hit preservation, crossing eraser splitting, large short-tap local gap behavior, and empty input stability.
- The test is independent from ImGui, SDL windows, and app startup.

Next recommended task:

- Continue splitting `AppDrawingMode.cpp`; the next likely candidate is direct onion/eraser preview drawing helpers, but inspect dependencies before moving them.

---

## Latest handoff: Phase 2 Step 2-z AppDrawingMode overlay split

Current state:

- Added `src/ui/AppDrawingModeOverlay.h/.cpp`.
- Moved direct onion-skin overlay drawing and lightweight eraser preview drawing out of `AppDrawingMode.cpp`.
- `AppDrawingMode.cpp` still owns App state, input handling, frame selection, and renderer dirty notifications.
- Debug app build succeeded after the split.

Next recommended task:

- Continue reducing `AppDrawingMode.cpp`; inspect timesheet playback/helper logic near the top next, or add a small non-renderer test if a helper group is isolated.

---

## Latest handoff: Phase 2 Step 2-aa AppDrawingMode timesheet split

Current state:

- Added `src/ui/AppDrawingModeTimesheet.h/.cpp`.
- Moved drawing-mode timesheet helpers out of `AppDrawingMode.cpp`.
- The helpers cover entry counting, selected panel entry lookup, project cell lookup, missing drawing-frame creation, and playback order navigation.
- `AppDrawingMode.cpp` still owns App state, UI rendering flow, and mode transitions.
- Debug app build succeeded after the split.

Next recommended task:

- Add a focused selftest for `AppDrawingModeTimesheet`, especially playback order and missing frame creation, then continue reducing `AppDrawingMode.cpp`.

---

## Latest handoff: Phase 2 Step 2-ab AppDrawingMode timesheet selftest

Current state:

- Added `tools/app_drawing_mode_timesheet_selftest.cpp`.
- Added CMake target `perapera_app_drawing_mode_timesheet_selftest`.
- The selftest covers entry counting, selected panel entry lookup, project cell lookup, missing drawing-frame creation, and playback-order navigation.
- The test is independent from ImGui drawing, SDL windows, and app startup.

Next recommended task:

- Continue reducing `AppDrawingMode.cpp` toward the 800-line target, or add selftests for any new helper group before moving more code.

---

## Latest handoff: Phase 2 Step 2-ag AppDrawingMode panel split

Current state:

- Added `src/ui/AppDrawingModePanels.cpp`.
- Moved `App::drawLeftSidebar()`, `App::drawRightSidebar()`, and `App::drawTimelineArea()` out of `AppDrawingMode.cpp`.
- Kept panel actions, preview readiness reset, renderer dirty/clear calls, save/load/export routing, and timeline playback-order behavior unchanged.
- This is a structure-only split; it does not add startup or project-load work.

Next recommended task:

- Continue reducing `src/ui/AppDrawingMode.cpp` with another closed helper group, or add a focused selftest if the next extracted logic is model-only.

---

## Latest handoff: Phase 2 Step 2-ah AppDrawingMode canvas split

Current state:

- Added `src/ui/AppDrawingModeCanvas.cpp`.
- Moved `App::drawCanvasArea()`, `App::fitCanvasToArea()`, `App::finishStroke()`, `App::cancelStroke()`, and `App::removeIntersectingStrokes()` out of `AppDrawingMode.cpp`.
- Kept canvas display behavior, timesheet preview rendering, onion/light-table overlays, MyPaint stroke commit, eraser splitting, and renderer dirty/cache calls unchanged.
- This step is structure-only and does not add startup or project-load work.

Next recommended task:

- `src/ui/AppDrawingMode.cpp` is now near the 900-line range. Continue toward the 800-line guideline by splitting a closed timesheet assistant UI block, or add a focused helper/test if the next block can be made model-only.

---

## Latest handoff: Phase 2 Step 2-ai Timesheet playback helper split

Current state:

- Added `src/ui/AppDrawingModeTimesheetPlayback.cpp`.
- Moved selected-T preview movement, range playback, ping-pong playback stepping, and edit-target synchronization out of local lambdas in `AppDrawingMode.cpp`.
- `src/ui/AppDrawingMode.cpp` is now under the 800-line guideline.
- Behavior should remain unchanged: renderer invalidation, selected T/F handling, and playback messages are preserved.

Next recommended task:

- Add focused tests for the new playback helper behavior if practical, or continue separating the remaining timesheet assistant UI into smaller files without changing project load/startup behavior.
