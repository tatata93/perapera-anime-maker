# final_spec_v6 Phase 2 Step 2-i cleanup: remove broken DrawingNewLayoutIO path

## Purpose
Remove the broken temporary DrawingNewLayoutIO path before moving forward.

## Reason
The target `perapera_drawing_new_layout_io_selftest` was created, but it tried to compile an app-facing drawing IO layer that included `drawing/DrawingCell.h`. In the current local build path this header was not available to that target, so the selftest failed before runtime.

The main executable still built, so the issue was isolated to the added Step 2-i selftest/app bridge. Keeping dead or duplicate IO paths would make the repository heavier and more fragile.

## Decision
Remove:

- `src/project/DrawingNewLayoutIO.h`
- `src/project/DrawingNewLayoutIO.cpp`
- `tools/drawing_new_layout_io_selftest.cpp`
- broken Step 2-i CMake entries
- old Step 2-i policy notes

Keep:

- Phase 2 Step 2-a to 2-h layout path / save / read / load entries
- the main executable build path

## Next work
Reconnect app save/load only after inspecting the current app code path. Do not add another parallel IO layer unless it is clearly needed.
