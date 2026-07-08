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
