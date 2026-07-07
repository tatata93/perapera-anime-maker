# final_spec_v6 Phase 2 Step 2-e: project layout save entry

Goal:

- Add one small save entry point for the new `scenes/scene_001/cuts/cut_001/` layout.
- Reuse existing IO helpers instead of adding save logic to UI files.
- Keep Scene Plate / ScenePlateImageCache / scene panel code removed.

Implemented entry:

- `src/io/ProjectLayoutSaveEntry.h`
- `src/io/ProjectLayoutSaveEntry.cpp`
- `tools/project_layout_save_entry_selftest.cpp`

The entry point writes:

- `scene.json`
- `cut.json`
- `timesheet.json`
- `cells/<cellId>/cell.json`
- `cells/<cellId>/frames/frame_NNN/frame.json`
- `cells/<cellId>/frames/frame_NNN/layers/layer_NNN.json`

Next step:

- Connect the entry to a controlled save path only after build and selftests stay green.
