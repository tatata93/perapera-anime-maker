# final_spec_v6 Phase 2 Step 2-o: project metadata and motion keys

- Save root `project.json` from the new layout save entry when a full `Project` is available.
- Load `project.json` into canvas, output, audio, and camera settings without restoring legacy `ProjectIO`.
- Keep scene/cut metadata authoritative for active cut timing while preserving project output dimensions.
- Save and load `Cell.motionKeys` in `cell.json`.
- Cover the new fields in the layout load selftest.
