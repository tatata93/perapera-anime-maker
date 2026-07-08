# Phase 2 Step 2-z: AppDrawingMode overlay split policy

- Split direct onion-skin overlay drawing and lightweight eraser preview drawing into `AppDrawingModeOverlay`.
- Kept App state, frame selection, input handling, and renderer dirty notifications in `AppDrawingMode.cpp`.
- Reduced `AppDrawingMode.cpp` while preserving current drawing workflow behavior.
- This continues the large drawing-mode file reduction using closed helper groups.
