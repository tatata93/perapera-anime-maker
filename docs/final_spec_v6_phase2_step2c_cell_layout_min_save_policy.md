# final_spec_v6 Phase 2 Step 2-c: Cell Layout Minimal Save Policy

## Position

This is part of final_spec_v6 Phase 2 Step 2.

Step 2-a defined the new project layout paths.
Step 2-b added minimal scene.json / cut.json saving.
Step 2-c adds the first cell-level layout writer.

## Decision

Because this project is still in development, legacy save compatibility is not a blocker.
The new final_spec_v6 layout is preferred over preserving old ProjectIO behavior.

This step writes:

```text
scenes/scene_001/cuts/cut_001/cells/cell_ID/cell.json
scenes/scene_001/cuts/cut_001/cells/cell_ID/frames/
scenes/scene_001/cuts/cut_001/cells/cell_ID/frames/frame_001/frame.json
```

## Scope

This step does not replace the application save button yet.
It adds reusable IO helpers and a selftest.

## Non-goals

- No UI change.
- No AppDrawingMode.cpp change.
- No Scene Plate reintroduction.
- No legacy ProjectIO compatibility layer.
- No layer_NNN.json stroke save migration yet.

## File size policy

New files are intentionally small.
Large UI files should not receive new save logic.
If a file grows beyond roughly 800 lines, split or lighten it before adding more features unless there is a strong reason not to.
