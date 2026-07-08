# Phase 2 Step 2-ae: drawing workspace layout helper split

- Moved the DrawingWorkspace ImGui layout shell out of AppDrawingMode.cpp.
- Kept App state ownership, input flow, renderer invalidation, and save/load untouched.
- Did not restore DrawingNewLayoutIO.
- Did not add a new selftest target because this is UI layout plumbing.