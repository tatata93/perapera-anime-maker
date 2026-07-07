# ChatGPT handoff

Current branch/state:

- Work is on `main`.
- GitHub should have only `main`.
- Latest completed step: final_spec_v6 Phase 2 Step 2-f.

What was just done:

- Added Step 2-e save entry earlier:
  - `src/io/ProjectLayoutSaveEntry.h`
  - `src/io/ProjectLayoutSaveEntry.cpp`
  - `tools/project_layout_save_entry_selftest.cpp`
- Connected Step 2-f app save:
  - `src/ui/AppProjectIO.cpp`
  - `App::saveProject()` now writes both the legacy save and the new `scenes/scene_001/cuts/cut_001/` layout.
  - `App::saveLoadRoundTripCheck()` also writes the new layout, then still verifies through the existing legacy load path.

Important direction:

- Do not revive Scene Plate, ScenePlateImageCache, or the old scene panel.
- Background, layout, BOOK, effect, and reference should remain normal Cells timed by the Timesheet.
- Backward compatibility with old project files is not required during development.
- Keep UI changes small until the new save/load path is stable.

Verification already done:

- `cmake --build .\build --config Debug --target perapera_anime_maker --parallel 1`
- All `build/bin/*selftest.exe` passed: 16 selftests.

Recommended next task:

- Phase 2 Step 2-g: add a narrow read/inspection or load selftest for the new layout.
- Do not replace the app load path yet unless the new layout read path is verified first.

User confirmation that would help:

- Launch `build/bin/perapera_anime_maker.exe`.
- Use Save once.
- Confirm the project folder now contains `scenes/scene_001/cuts/cut_001/` with `cut.json`, `timesheet.json`, and `cells/`.
