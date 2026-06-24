# WORK_LOG

## Phase 1.5 Step 14c: MyPaint realtime stroke path

### 目的
- libmypaint の重い「確定時一括 bakeStroke(stroke全点)」をやめる。
- ドラッグ中に `mypaint_brush_stroke_to()` へ1点ずつ投入し、dabを即時CanvasBitmapへ描く。
- ペンを離した瞬間にアプリが止まる根本原因を取り除く。

### 変更ファイル
- `src/brush/MyPaintBrushEngine.h`
- `src/brush/MyPaintBrushEngine.cpp`
- `src/render/CanvasRenderer.h`
- `src/render/CanvasRenderer.cpp`
- `src/ui/App.h`
- `src/ui/AppInput.cpp`
- `src/ui/AppDrawingMode.cpp`
- `WORK_LOG.md`
- `DECISIONS.md`

### 実装内容
- `MyPaintBrushEngine` に `beginStroke()` / `addPoint()` / `endStroke()` を追加。
- libmypaintのブラシ状態を `MyPaintBrushEngine::MyPaintState` として保持。
- `addPoint()` で新しい点だけを `mypaint_brush_stroke_to()` へ投入。
- `bakeStroke()` は保存データ再構築・libmypaint未使用時向けのSimple互換フォールバックへ変更。
- `shouldUseFastSimpleFallback` と `bakeSimpleContinuityCore` を削除。
- `CanvasRenderer::activeBitmap()` を追加し、MyPaintが直接描く対象Bitmapを取得できるようにした。
- `CanvasRenderer::markActiveBitmapClean()` を追加し、逐次描画済みBitmapとProject側点列のrevisionを同期。
- `AppInput.cpp` でMyPaint選択時だけドラッグ開始/更新時に逐次描画を呼ぶよう変更。
- `AppDrawingMode.cpp` でMyPaint逐次描画中はDrawListプレビューを重ねないよう変更。
- MyPaintストローク確定時は、ピクセルは既に描画済みなので `markAllDirty()` を呼ばず、点列だけ保存する。
- キャンセル時は未確定の直接描画ピクセルを破棄するため `markAllDirty()` で再構築する。

### 未実行
- この環境ではWindows / SDL3 / vcpkg / libmypaint 実ビルドは未実行。

## Phase 1.5 Step 14d: MyPaintState incomplete type delete fix

- `MyPaintBrushEngine` の `std::unique_ptr<MyPaintState>` を custom deleter 付きに変更した。
- `MyPaintStateDeleter::operator()` を `MyPaintBrushEngine.cpp` の `MyPaintState` 定義後に実装した。
- MSVC C4150（前方宣言のみの型を delete する警告）を避けるためのビルド修正。
- libmypaint逐次描画の実装方針・挙動は変更していない。
