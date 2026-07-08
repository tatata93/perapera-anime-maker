# Phase 2 Step 2-af: preview helper residue cleanup

- Ensures previewFrameWithEraser lives in AppDrawingModePreview.
- Removes any leftover anonymous previewFrameWithEraser definition from AppDrawingMode.cpp.
- Keeps input flow, renderer invalidation, save/load, and DrawingNewLayoutIO untouched.