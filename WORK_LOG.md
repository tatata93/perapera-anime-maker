# WORK_LOG - Phase 1 Step 1-4 runtime fix v10

## 目的

Phase 1 Step 1-4 の作画UI配線後に発生していた以下を、コード全体の整合を取り直して修正する。

- フレームを追加するとキャンバス表示が不自然に切り替わる。
- 1つのフレームへ描いた線が他フレームにも見える。
- MP4出力失敗時にFFmpeg本体のエラーがログに残らない。
- Dear ImGui のID衝突を避けるため、ポインタID依存を減らす。

## 修正内容

### フレーム別キャンバス修正

- `CanvasRenderer` のBitmapキャッシュキーを `layerIndex` だけから、`Frame* + layerIndex` に変更した。
- これにより、同じレイヤー番号でも別フレームのBitmapを共有しない。
- `finishStroke()` では即時Bitmap焼き込みをやめ、Project内のストローク点列を正本として、`markAllDirty()` 後に次回drawで再構築するよう変更した。
- オニオンスキンが通常描画と誤認されやすいため、初期値をOFFにした。

### フレーム操作修正

- `addFrame()` は空白フレームを現在フレームの後ろへ挿入するが、表示中フレームを勝手に切り替えない。
- 新しい空白フレームへ描く場合はタイムラインから選択する。
- `FramePanel` / `LayerPanel` / `TimelinePanel` / `ExportPanel` のポインタ `PushID` を廃止し、固定IDに整理した。

### MP4出力修正

- Windowsでは `cmd` / batch を経由せず、`CreateProcessW` でFFmpegを直接起動するようにした。
- `frame_%03d.png` の `%` がbatchで壊れる問題を避けた。
- FFmpegの標準出力・標準エラーを `exports/mp4/ffmpeg_last_run.log` へ直接リダイレクトするようにした。

## 変更ファイル

- `src/ui/App.h`
- `src/ui/App.cpp`
- `src/ui/AppDrawingMode.cpp`
- `src/ui/AppOperations.cpp`
- `src/ui/panels/FramePanel.cpp`
- `src/ui/panels/LayerPanel.cpp`
- `src/ui/panels/TimelinePanel.cpp`
- `src/ui/panels/ExportPanel.h`
- `src/ui/panels/ExportPanel.cpp`
- `src/render/CanvasRenderer.h`
- `src/render/CanvasRenderer.cpp`
- `src/render/CanvasBitmap.h`
- `src/render/CanvasBitmap.cpp`
- `src/brush/BrushEngine.h`
- `src/brush/SimpleBrushEngine.h`
- `src/brush/SimpleBrushEngine.cpp`
- `src/io/FfmpegRunner.cpp`
- `WORK_LOG.md`

## ビルド確認

この環境ではWindows/MSVC/SDL3/ImGui実ビルドは実行していない。
ただし、ヘッダと実装の不一致を避けるため、過去に不整合が出た `CanvasBitmap` / `BrushEngine` も同梱している。
