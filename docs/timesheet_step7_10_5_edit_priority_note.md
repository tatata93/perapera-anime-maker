# Timesheet Rebuild Step 7.10.5: edit-priority display when timeline F differs from sheet T

## Purpose

Step 7.10 allowed drawing even when the selected timesheet T and the selected drawing frame were different, but it still displayed the timesheet-resolved drawing frame underneath the edited drawing frame. This made timeline-driven drawing confusing because the user saw both the selected drawing frame and the sheet-derived drawing frame at the same time.

Step 7.10.5 changes the default behavior:

- When the selected timesheet T resolves to a different drawing frame than the active editing frame, the canvas prioritizes the active editing frame.
- The timesheet-derived drawing frame is hidden in that mismatch case.
- The old overlay behavior remains available through an optional compatibility checkbox.

## Policy

- Timesheet T selection controls preview/playback.
- Timeline drawing-frame selection controls editing.
- If those two disagree during manual editing, the canvas should not show two competing drawing frames by default.
- The selected editing drawing frame is the one the user can draw on and should be visually dominant.

## UI changes

In the timesheet playback window, the drawing-target section now includes:

- `T選択で編集対象も追従`
- `今のTの作画Fを編集`
- `ズレたら編集中Fだけ表示`

If `ズレたら編集中Fだけ表示` is off, the old overlay option appears:

- `旧方式: TS表示の上に編集中Fを重ねる`

## Canvas badge

When the timesheet preview is hidden because editing has priority, the canvas badge shows `TS EDIT` instead of `TS ON`.

