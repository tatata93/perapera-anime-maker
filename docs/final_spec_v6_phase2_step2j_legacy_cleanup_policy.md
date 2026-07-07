# final_spec_v6 Phase 2 Step 2-j: Legacy drawing IO cleanup

## Purpose

Remove the failed `DrawingNewLayoutIO` path and stale Step 2-i policy documents.
The attempted drawing-side adapter introduced an include dependency on a drawing header path that is not valid for the selftest target.
Since the application executable can still be generated, the failed adapter path should be removed rather than repaired in parallel.

## Policy

- Keep the new layout IO core path: `ProjectLayoutSaveEntry`, `ProjectLayoutReadEntry`, `ProjectLayoutLoadEntry`, `SceneCutLayoutIO`, `CellLayoutIO`, and `LayerLayoutIO`.
- Do not keep broken duplicate drawing IO adapters.
- Do not keep stale policy documents for failed approaches.
- Avoid adding code to large UI files until the exact app save/load function is confirmed.
- Continue Phase 2 with a lightweight review before app load/save reconnection.

## Removed path

- `src/project/DrawingNewLayoutIO.h`
- `src/project/DrawingNewLayoutIO.cpp`
- `tools/drawing_new_layout_io_selftest.cpp`
- stale Step 2-i policy documents from failed attempts

## Next work

Inspect the actual app save/load call sites and connect them directly to the existing new layout entry points, or remove old save/load code only after the live call path is confirmed.
