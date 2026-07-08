# Phase 2 Step 3-a: Cut camera metadata bridge policy

- Add `CameraSettings` as a shared core model so both `Project` and `Cut` can refer to the same camera data shape.
- Add optional Cut-owned camera metadata to `Cut` and `cut.json`.
- Keep old cut files safe by using `Cut::hasCamera`; if a loaded cut has no camera object, app load should not overwrite the Project-level camera.
- Connect app save so the active Cut receives the current Project camera.
- This is a small bridge toward the Phase 2 requirement that Timesheet / Cell / Camera are connected at Cut level.
