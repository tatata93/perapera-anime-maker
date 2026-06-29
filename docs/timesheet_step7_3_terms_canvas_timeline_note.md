# Timesheet Rebuild Step 7.3: terms, canvas wheel, timeline boxes

## Purpose

Fix three usability problems before continuing export integration.

1. The canvas wheel zoom also scrolled the parent workspace.
2. The drawing-frame boxes in the bottom timeline were too easy to hide or clip.
3. Frame / koma / drawing number terms were becoming ambiguous.

## Changes

- The bottom timeline now shows the drawing-frame box strip first.
- Timeline boxes are labeled `作F1`, `作F2`, ... to indicate drawing-frame slots.
- Timeline add/duplicate/delete buttons are labeled as `作画F` operations.
- The main workspace and upper drawing area disable mouse-wheel scrolling, so canvas wheel input does not scroll the whole software.
- Canvas wheel input is consumed after zooming.
- The canvas overlay remains small and keeps `TS ON/OFF`, `T`, `編集F`, `表示セルN`, and `entries`.

## Terminology decision

Use these terms consistently in UI and documentation:

- `T`: a time position in the timesheet. Example: `T1`, `T2`, `T3`.
- `作画F`: the editable drawing-frame slot inside a cell. Example: `作画F1`, `作画F2`.
- `コマ数`: duration / count of time positions. Example: `3コマ` means three T positions of length, not three drawing frames.
- `フレーム`: internal/video frame concept only. Avoid using it alone in the UI when `T` or `作画F` is meant.

## Not changed

- Timesheet saving/loading.
- Timesheet playback logic.
- PNG/MP4 export.
- Project save integration.
