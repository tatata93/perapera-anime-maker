# Phase 2 Step 2-w: CanvasRenderer support selftest policy

- Added a lightweight selftest for `CanvasRendererSupport`.
- Covered display opacity caps, display layer rank, cache ID fallback, lightweight layer revision values, and Paint append-order safety.
- Kept the test independent from app startup and renderer windows so it stays cheap.
- This guards the Step 2-v split before deeper renderer or drawing-mode changes.
