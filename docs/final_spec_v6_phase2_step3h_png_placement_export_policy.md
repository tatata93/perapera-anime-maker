# Phase 2 Step 3-h: PNG placement export policy

Purpose:

- Make PNG sequence / MP4 pre-export follow the same CellPlacement x/y/scale behavior as the canvas preview.
- Keep startup and project loading light.

Implementation:

- `PngExporter` now blends each Cell image using the Cell's `placement.x`, `placement.y`, and `placement.scale`.
- Timesheet export already copies resolved placement into temporary per-frame Cells, so this also applies to Timesheet PNG sequence and MP4 pre-export frames.
- Rotation is still deferred.

Constraints:

- `src/io/PngExporter.cpp` remains under the 800-line guideline.
- No preview-file generation, project-load scanning, or startup warmup was added.

Verification:

- `cmake -S . -B .\build -G "Visual Studio 18 2026"` succeeded.
- `perapera_png_timesheet_exporter_selftest` built and passed.
- Debug `perapera_anime_maker` build succeeded.
- `build/bin/perapera_anime_maker.exe` exists.

Next recommended step:

- Phase 2 Step 3-i: Timesheet UI usability pass, then Phase 2 closeout audit.
