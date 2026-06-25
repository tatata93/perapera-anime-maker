# WORK_LOG

## Phase 1.5 Step 14h: reapply MyPaint realtime App connection

### Goal
Ensure MyPaintBrushEngine is explicitly connected to the App input lifecycle while preserving Step 14g MyPaint replay/restore behavior.

### Changed / Reapplied files
- `src/ui/App.h`
- `src/ui/AppInput.cpp`
- `src/ui/AppDrawingMode.cpp`
- `src/render/CanvasRenderer.h`
- `src/render/CanvasRenderer.cpp`
- `src/brush/MyPaintBrushEngine.h`
- `src/brush/MyPaintBrushEngine.cpp`

### Summary
- Added/kept `MyPaintBrushEngine myPaintEngine_` in `App`.
- Added/kept `bool isMyPaintStrokeActive_` in `App`.
- `beginStroke()` starts MyPaint realtime drawing when MyPaint engine is selected.
- `updateStroke()` feeds new filtered points to `myPaintEngine_.addPoint()`.
- `finishStroke()` skips final `bakeStroke` for realtime MyPaint strokes and only stores the point list.
- Added/kept `CanvasRenderer::bitmapForLayerPtr()` for direct bitmap access.
- Kept Step 14g replay behavior so saved/rebuilt MyPaint strokes are replayed through MyPaint instead of returning to Simple/default appearance.

### Build note
Use full clean vcpkg build only for verification.
