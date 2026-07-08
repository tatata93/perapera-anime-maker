# final_spec_v6 Phase 2 Step 2-ac preview helper split policy

- Split the closed eraser preview frame helper out of `src/ui/AppDrawingMode.cpp`.
- Add `src/ui/AppDrawingModePreview.h/.cpp` as a narrow model-only helper group.
- Keep `App` state ownership, input flow, renderer invalidation, and save/load routing unchanged.
- Reuse the existing `AppDrawingModeEraser` helper instead of restoring `DrawingNewLayoutIO`.
- Add `perapera_app_drawing_mode_preview_selftest` to verify preview generation without mutating the source frame.
