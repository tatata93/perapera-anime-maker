# Phase 2 Step 3-g: CellPlacement visual preview policy

Purpose:

- Make resolved `CellPlacement` visible in the canvas preview.
- Keep the step small by applying x/y/scale only.

Implementation:

- `CanvasRenderer::draw(...)` can receive an optional `CellPlacement`.
- Bitmap drawing applies placement x/y/scale before adding the ImGui image.
- Timesheet scene preview passes `ResolvedTimesheetSceneCell::placement` to the renderer.
- Non-timesheet editing display uses each Cell's base placement.

Scope:

- Rotation is deferred.
- PNG export transform is deferred to avoid growing `PngExporter.cpp` in this step.
- No startup preview generation or project-load scanning was added.

Verification:

- `cmake -S . -B .\build -G "Visual Studio 18 2026"` succeeded.
- Debug `perapera_anime_maker` build succeeded.
- `build/bin/perapera_anime_maker.exe` exists.

Next recommended step:

- Phase 2 Step 3-h: decide whether to add PNG export placement transform or move to Timesheet UI usability.
