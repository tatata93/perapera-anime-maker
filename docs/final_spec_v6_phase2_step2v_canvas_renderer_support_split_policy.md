# Phase 2 Step 2-v: CanvasRenderer support split policy

- Split pure CanvasRenderer helper logic into `CanvasRendererSupport`.
- Moved display opacity, display layer rank, cache ID, lightweight revision, hash, alpha, and stroke bake helper functions.
- Kept cache ownership, progressive rebuild state, pruning, draw flow, and direct app-facing renderer methods in `CanvasRenderer.cpp`.
- Reduced `CanvasRenderer.cpp` to stay within the 800-line limit while preserving current runtime behavior.
