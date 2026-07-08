# Phase 2 Step 2-ah: AppDrawingMode canvas split policy

- Split canvas drawing, canvas fitting, stroke finish/cancel, and eraser rewrite methods from `AppDrawingMode.cpp` into `AppDrawingModeCanvas.cpp`.
- Keep the drawing behavior, renderer cache invalidation, MyPaint commit behavior, eraser splitting, and timesheet preview overlay behavior unchanged.
- Do not add project-load scanning, preview file generation, or startup-time warmup work.
- This step continues the large-file reduction needed before deeper timesheet and drawing UI changes.
