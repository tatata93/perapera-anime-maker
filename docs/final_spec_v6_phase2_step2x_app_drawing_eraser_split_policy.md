# Phase 2 Step 2-x: AppDrawingMode eraser split policy

- Split pure eraser stroke-splitting logic from `AppDrawingMode.cpp` into `AppDrawingModeEraser`.
- Kept onion-skin drawing, eraser preview drawing, App state updates, and renderer dirty notifications in `AppDrawingMode.cpp`.
- Reduced `AppDrawingMode.cpp` without changing the drawing workflow.
- This prepares the large drawing-mode file for further low-risk splits and focused tests.
