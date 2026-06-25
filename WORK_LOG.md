# WORK_LOG - Phase 1.5 Step 14f

## Summary
- MyPaintBrushEngine を App 入力経路へ明示接続した。
- App::beginStroke / updateStroke / finishStroke の逐次処理フローを整理した。
- CanvasRenderer に bitmapForLayerPtr() を追加し、MyPaint逐次処理が対象レイヤーの CanvasBitmap を取得できるようにした。
- SimpleBrushEngine 側は従来通り確定時 bakeStrokeOnLayer() 経由で処理する。

## Changed files
- src/ui/App.h
- src/ui/AppInput.cpp
- src/ui/AppDrawingMode.cpp
- src/render/CanvasRenderer.h
- src/render/CanvasRenderer.cpp
- src/brush/MyPaintBrushEngine.h
- src/brush/MyPaintBrushEngine.cpp

## Notes
- 現行ファイル分割では finishStroke() は AppInput.cpp ではなく AppDrawingMode.cpp に存在するため、同等修正を AppDrawingMode.cpp に入れた。
- MyPaintBrushEngine.cpp は直前の custom deleter / state_.reset 修正を含む版を同梱した。
