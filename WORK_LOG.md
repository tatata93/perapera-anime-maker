# WORK_LOG

## Phase 1 Step 1-4 runtime fix v4

### 目的

- フレーム追加ボタン hover 時に出る Dear ImGui の conflicting ID 警告を解消する。
- フレーム追加・削除ボタンを確実に動作させる。
- MP4 書き出しで FFmpeg 実行前に失敗しても `exports/mp4/ffmpeg_last_run.log` を必ず作る。
- ExportPanel の実行結果欄にログパスを含む失敗理由を表示する。

### 変更

- `src/ui/panels/FramePanel.cpp`
  - ボタンIDを `PushID` で完全分離。
  - `###` ラベル依存をやめ、表示名とIDを別スタックで管理。
- `src/ui/panels/LayerPanel.cpp`
  - レイヤー操作ボタンIDを完全分離。
- `src/ui/panels/TimelinePanel.cpp`
  - タイムライン操作ボタンIDを完全分離。
- `src/ui/AppDrawingMode.cpp`
  - 右サイドバー内のIDスタックを追加。
  - 右サイドバーの縦横スクロール指定を維持。
- `src/ui/panels/ExportPanel.cpp`
  - ExportPanel内の入力欄・ボタンIDを完全分離。
  - MP4ボタン直下の実行結果欄を維持。
- `src/ui/App.cpp`
  - `exportMp4()` の最初にMP4プリフライトログを作成。
  - PNG連番作成で失敗した場合も `ffmpeg_last_run.log` に理由を書く。

### 確認

- フレーム追加ボタンをhoverしてもDear ImGuiのID警告が出ないこと。
- フレーム追加でフレーム数が増えること。
- 2フレーム以上で削除ボタンが有効になり、削除できること。
- MP4書き出しに失敗しても `exports/mp4/ffmpeg_last_run.log` が生成されること。
