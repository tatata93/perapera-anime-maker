# Phase 2 Step 3-e: Cell order resolver policy

Purpose:

- Make display and export follow the active Cut cell order represented by `Project.cellOrder`.
- Keep `Cut.cellZOrderKeys` connected through the current migration bridge, where save writes `project.cellOrder` to `cut.cellZOrderKeys` and load restores it into `project.cellOrder`.

Implementation:

- Added `resolvedProjectCellOrderIndices(...)` in `CellOrderResolver.h`.
- The resolver uses `Project.cellOrder` first, skips missing/duplicate ids, then appends any remaining physical cells.
- Canvas visible-cell display now iterates through the resolved order.
- PNG/MP4 visible-cell export and Timesheet export input construction now iterate through the same resolved order.

Scope:

- This step connects the current default Cut cell order.
- True frame-varying `cellZOrderKeys` keyframes still need a later data-model/UI step if the project stores multiple keyed orders per timeline T.

Verification:

- `perapera_cell_order_resolver_selftest` builds and passes.
- Debug `perapera_anime_maker` builds and produces `build/bin/perapera_anime_maker.exe`.

Next recommended step:

- Phase 2 Step 3-f: connect Cell motion keys to resolved preview/export data.
