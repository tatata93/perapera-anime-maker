# Phase 2 Step 2-ag: AppDrawingMode panel split policy

- Split the drawing-mode side panels and bottom timeline area from `AppDrawingMode.cpp` into `AppDrawingModePanels.cpp`.
- Keep App state ownership, renderer invalidation, save/load/export calls, and panel action routing unchanged.
- Do not introduce new startup-time preview generation or project-load scanning.
- This step is a structure-only reduction for safer later UI and performance work.
- Continue reducing `AppDrawingMode.cpp` toward the 800-line guideline in later numbered Phase 2 steps.
