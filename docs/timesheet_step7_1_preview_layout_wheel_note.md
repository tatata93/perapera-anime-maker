# Timesheet Rebuild Step 7.1: preview visibility, timeline layout, canvas wheel safety

## Purpose

This step fixes three usability issues found after Step 7:

1. The canvas overlay did not clearly show `表示セルN`.
2. The bottom timeline layout was visually hard to read because light-table controls appeared before the frame strip.
3. Mouse wheel operation on the canvas zoomed the canvas but also scrolled the canvas host, making navigation unstable.

## Changes

- The canvas timesheet overlay now shows a clearer status line:
  - `TS表示 ON/OFF`
  - selected `T`
  - editing drawing frame `F`
  - `表示セルN=<count>`
  - `entries=<count>`
- The overlay is shown whenever the temporary timesheet has entries, even when the normal timeline playback temporarily disables timesheet preview.
- The bottom timeline now shows the frame strip first, then finger-playback and light-table controls below it.
- The drawing canvas child window now uses `ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse`, so mouse wheel on the canvas zooms without also scrolling the canvas host.

## Not changed

- Timesheet model format.
- Timesheet save/load format.
- Timeline playback logic.
- Timesheet playback logic.
- PNG/MP4 export.
- Project save/load integration.
