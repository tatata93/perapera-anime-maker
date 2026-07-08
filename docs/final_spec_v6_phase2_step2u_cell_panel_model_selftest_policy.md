# Phase 2 Step 2-u: CellPanel model selftest policy

- Added a lightweight selftest for `CellPanelModel`.
- Covered cell order rebuild, z order repair, unique cell IDs, display labels, duplicate names, and cell-scoped layer ID repair.
- Kept the test independent from ImGui and renderer code so it remains fast.
- This guards the Step 2-t split before larger UI or renderer files are divided.
