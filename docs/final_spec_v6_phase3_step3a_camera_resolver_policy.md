# Phase 3 Step 3-a: Camera Resolver Policy

Phase 3 starts with a lightweight 2D camera resolver before adding shooting UI, effects, or heavy preview generation.

Implemented policy:

- Add `src/core/CameraResolver.h/.cpp`.
- Resolve `CameraSettings` to a `ResolvedCamera2D` for a timeline frame.
- Keep the resolver pure and independent of UI, PNG writing, SDL, ImGui, and project loading.
- If camera animation is disabled, use the base camera values.
- If camera animation is enabled:
  - before the first key, hold the first key;
  - after the last key, hold the last key;
  - between keys, linearly interpolate center and zoom;
  - exact duplicate frame keys use the last matching key.
- Clamp zoom to a small positive minimum so later preview/export math cannot divide by zero.

Deferred:

- Applying camera transforms to canvas preview.
- Applying camera transforms to PNG sequence / MP4 pre-export.
- Shooting-mode UI controls.
- Multiplane depth, depth of field, and effects.
