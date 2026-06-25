# WORK_LOG

## Phase 1.5 Step 15: Export modes for line test / color trace / line-only

- Added `ExportMode` to `PngExporter`.
- Added export mode selector to `ExportPanel`.
- Wired active PNG, PNG sequence, and MP4 pre-export PNG generation to the selected mode.
- Added white-background output for Composite / LineTest / ColorTrace.
- Added transparent-background output for LineOnly.
- Added layer filtering: Normal only, Normal+ColorTrace, or all visible layers according to the mode.

Build note: Windows/vcpkg/SDL full build must be run by the user environment.
