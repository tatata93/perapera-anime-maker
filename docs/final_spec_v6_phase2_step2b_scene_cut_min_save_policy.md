# final_spec_v6 Phase 2 Step 2-b: scene.json / cut.json minimal save policy

## Scope

This step adds a minimal writer for the future folder layout:

```text
scenes/scene_001/scene.json
scenes/scene_001/cuts/cut_001/cut.json
scenes/scene_001/cuts/cut_001/timesheet.json
scenes/scene_001/cuts/cut_001/cells/
```

## Rules

- Do not replace current ProjectIO yet.
- Do not move existing Cell / Frame / Layer data yet.
- Do not add UI.
- Do not touch AppDrawingMode.cpp.
- Do not reintroduce Scene Plate or a scene-management panel.
- Keep Timesheet as a physical `timesheet.json` file under the Cut folder.

## Implementation notes

`SceneCutLayoutIO` writes `scene.json` itself and delegates Cut metadata and Timesheet writing to existing `CutIO::saveCut`.
This keeps the Cut and Timesheet formats consistent with the existing CutIO layer.

## Next step

Phase 2 Step 2-c should prepare cell folder metadata such as `cell.json`, but still avoid replacing existing Project save/load until a compatibility selftest exists.
