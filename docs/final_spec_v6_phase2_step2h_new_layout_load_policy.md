# final_spec_v6 Phase 2 Step 2-h: New layout load entry

## Purpose

Add a small load entry for the new Phase 2 project layout:

- `scenes/scene_001/scene.json`
- `scenes/scene_001/cuts/cut_001/cut.json`
- `scenes/scene_001/cuts/cut_001/timesheet.json`
- `scenes/scene_001/cuts/cut_001/cells/cell_ID/cell.json`
- `frames/frame_001/frame.json`
- `layers/layer_001.json`

## Policy

This project is still in development, so old save compatibility is not a blocker.
The loader targets the new `final_spec_v6` layout first.

## Lightweight rule

Do not add load logic to large UI files. Keep load code in `src/io/ProjectLayoutLoadEntry.*`.
Avoid duplicate path helpers; reuse `ProjectLayoutPaths.h`.

## Claude cleanup

`CLAUDE_HANDOFF.md` is removed for now because external handoff is no longer needed.
