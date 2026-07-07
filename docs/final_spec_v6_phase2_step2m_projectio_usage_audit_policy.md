# Phase 2 Step 2-m ProjectIO usage audit policy

This step audits whether the old `ProjectIO` route can be removed.

Rules:

- Do not reintroduce `DrawingNewLayoutIO`.
- Do not delete `ProjectIO.*` while references remain outside `src/io/ProjectIO.*`.
- If all references are gone, delete `ProjectIO.h`, `ProjectIO.cpp`, and matching CMake source entries.
- Keep the new layout IO path: `ProjectLayoutSaveEntry`, `ProjectLayoutLoadEntry`, `SceneCutLayoutIO`, `CellLayoutIO`, and `LayerLayoutIO`.
- Keep files under 800 lines where practical. Split or remove unnecessary code before expanding large files.
