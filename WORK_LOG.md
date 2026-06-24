# WORK_LOG

## Phase 1 Step 1-4 stability fix v13

### Scope
- Fix onion skin visibility.
- Fix eraser behavior so it splits/removes only the touched portion instead of deleting a whole stroke.

### Changed files
- src/render/CanvasRenderer.h
- src/render/CanvasRenderer.cpp
- src/ui/AppDrawingMode.cpp

### Notes
- Onion skin had two problems: CanvasRenderer::draw repainted the canvas background after onion skin had been drawn, and black strokes did not become blue/red by simple ImGui texture tinting.
- v13 removes the renderer-side background fill and bakes onion skins into dedicated blue/red bitmap caches.
- Eraser now resamples affected strokes and splits them into remaining stroke parts.
- No CMakeLists.txt changes.
