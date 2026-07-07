# final_spec_v6 Phase 2 Step 2-g: project layout read entry

Goal:

- Add a narrow read/inspection entry for the new `scenes/scene_001/cuts/cut_001/` layout.
- Verify the new layout can be parsed before replacing the app load path.
- Avoid broad UI changes.

Implemented entry:

- `src/io/ProjectLayoutReadEntry.h`
- `src/io/ProjectLayoutReadEntry.cpp`
- `tools/project_layout_read_entry_selftest.cpp`

The inspector reads and validates:

- `scene.json`
- `cut.json`
- `timesheet.json` through `CutIO::loadCut`
- `cells/<cellId>/cell.json`
- `cells/<cellId>/frames/frame_NNN/frame.json`
- `cells/<cellId>/frames/frame_NNN/layers/*.json`

Next step:

- Use this read entry as the basis for a real new-layout app load path.
