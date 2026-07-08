# Phase 2 Step 2-m ProjectIO usage audit

This report reflects the repository after Step 2-l connected `AppProjectIO.cpp`
directly to the new layout save/load entries.

## Current source state

- `src/io/ProjectIO.h`: absent.
- `src/io/ProjectIO.cpp`: absent.
- `CMakeLists.txt`: no longer lists `src/io/ProjectIO.cpp`.
- `src/ui/AppProjectIO.cpp`: no direct `ProjectIO::save` / `ProjectIO::load` calls.
- `DrawingNewLayoutIO`: not present in active source or CMake targets.

## Remaining references

Remaining `ProjectIO` / `DrawingNewLayoutIO` text is historical documentation,
work logs, decisions, or final spec notes. It is not an active build dependency.

## Files over 800 lines

- `src/ui/AppDrawingMode.cpp`: 1917 lines.
- `src/render/CanvasRenderer.cpp`: 1030 lines.
- `src/ui/panels/CellPanel.cpp`: 951 lines.

`src/io/ProjectIO.cpp` is no longer present, so it is no longer part of the
800-line file list.

## Action

No further `ProjectIO.*` deletion is required because those files are already
absent. Future work should focus on increasing new-layout load fidelity rather
than restoring the legacy route.
