# final_spec_v6 Phase 2 Step 2-i header-only stroke build fix

## Purpose

Fix the app drawing new-layout IO selftest registration after the local tree reported that `src/drawing/Stroke.cpp` does not exist.

## Decision

`DrawingNewLayoutIO` should not require `src/drawing/Stroke.cpp` for this selftest. The current save/load code only needs the drawing data containers and can avoid calling non-inline `Stroke` member functions.

## Changes

- Remove `stroke.invalidateBounds()` calls from `DrawingNewLayoutIO.cpp` and the selftest.
- Rebuild the CMake Step 2-i block without `src/drawing/Stroke.cpp`.
- Keep the implementation split out of large UI files.
- Do not touch legacy compatibility paths.
