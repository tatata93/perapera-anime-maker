# WORK LOG

## Phase 1 Step 1-4 stability pass v17

### Purpose
- Fix eraser behavior based on the actual current repository code path.
- Fix onion skin visibility by drawing onion strokes on the foreground draw list, clipped to the canvas area.

### Changes
- `src/ui/AppDrawingMode.cpp`
  - Added `distancePointToStrokePathSquared()` for a stable eraser-path distance test.
  - Reworked `splitStrokeByEraser()` to prevent a whole stroke from disappearing when every sampled point is hit.
  - If all sampled points would be erased, the eraser now cuts only a local gap around the closest hit.
  - Onion strokes are drawn via `ImGui::GetForegroundDrawList()` after the normal canvas texture draw.
  - Onion colors and minimum stroke widths were strengthened to make visibility unambiguous.
  - Runtime marker updated to `Step 1-4 stability pass v17`.

### Verification requested
- Build and launch.
- Confirm the right sidebar marker shows `Step 1-4 stability pass v17`.
- Draw a long stroke, erase the middle, and confirm only a local gap is removed.
- Create two frames, draw on frame 1, select frame 2, enable previous onion, and confirm frame 1 appears as a blue overlay.
