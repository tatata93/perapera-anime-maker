# final_spec_v6 Phase 2 Step 2-i: App new-layout drawing load

This step connects the drawing app load/save path to the new `scenes/scene_001/cuts/cut_001/cells/...` layout without using legacy compatibility as the main path.

## Decision

- Do not include `core/Cell.h` / `core/Stroke.h` directly from `DrawingCanvasPanel_ProjectIO.cpp`.
- The UI drawing model already has `drawing/Stroke.h`, which uses the same `perapera::Stroke` name as `core/Stroke.h`.
- To avoid a compile-time type collision, app load/save uses `project/DrawingNewLayoutIO.*`, which reads/writes the new layout directly into `DrawingCell` / `AnimationFrame` / `DrawingLayer`.

## Scope

- Add `DrawingNewLayoutIO.h/.cpp`.
- Patch `saveCurrentCutDrawingFile` and `loadCurrentCutDrawingFile` to use the new layout cell/frame/layer JSON.
- Keep large UI files lightweight: only replace two method bodies and add one include.
- No Claude handoff files.
- No legacy ProjectIO compatibility work.

