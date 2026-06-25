# DECISIONS

## Phase 1.5 Step 14h

### Decision
Reapply and package the App-side MyPaint realtime connection together with the Step 14g replay/restore changes.

### Reason
The MyPaint realtime stroke engine must be owned by `App`, started on drag begin, fed on drag update, and ended on drag finish. If the App-side connection is missing or overwritten by later packages, MyPaint strokes can fall back to commit-time/default behavior or fail to preserve brush appearance across undo/save/reload.

### Implementation policy
- Do not reintroduce commit-time full-stroke MyPaint baking for live drawing.
- Do not provide short/incremental build as the main verification path.
- Keep SimpleBrushEngine path intact.
- Keep saved/rebuilt MyPaint strokes replayed through MyPaint when libmypaint is available.
