# WORK LOG

## 2026-06-23 Phase 1 Step 1-4: Runtime fix v2 - frame add/delete and MP4 diagnostics

### 作業概要
- フレームの追加・削除が反応しない問題に対して、右パネルとタイムラインの両方から操作できるようにした。
- ImGuiボタンID衝突を避けるため、各ボタンの内部IDを `##FramePanel_*` / `##Timeline_*` / `##Export_*` に分離した。
- MP4出力失敗時に原因を追えるよう、FFmpeg実行ログを `exports/mp4/ffmpeg_last_run.log` に保存するようにした。

### 変更ファイル
- src/ui/panels/FramePanel.cpp
- src/ui/panels/TimelinePanel.h
- src/ui/panels/TimelinePanel.cpp
- src/ui/AppDrawingMode.cpp
- src/ui/AppOperations.cpp
- src/ui/App.cpp
- src/ui/panels/ExportPanel.cpp
- src/io/FfmpegRunner.cpp

### 実装内容
- フレーム操作ボタンを右パネルとタイムラインへ配置。
- フレーム追加・複製・削除後に `frame_001` 形式で名前を振り直す処理を追加。
- 削除は最後の1フレームだけになった場合は無効化し、プロジェクトがフレーム0枚にならないようにした。
- MP4出力前にPNG連番を必ず生成し、`frame_001.png` の存在確認後にFFmpegを実行。
- FFmpegコマンドの標準出力・標準エラーをログファイルへ追記。
- `ffmpeg` のようなPATH検索名は引用符なし、フルパスは引用符付きで実行するよう調整。

### 未完了
- ファイル選択ダイアログは未実装。
- MP4失敗時のログ本文をアプリ内に表示する機能は未実装。

### 次にやること
- 実機でフレーム追加・削除・MP4出力を確認する。
- 失敗時は `exports/mp4/ffmpeg_last_run.log` の内容を見て原因を潰す。

### 判断待ち（私への確認事項）
- 消しゴムを「ストローク削除方式」のまま進めるか、「消しゴムストロークを保存する方式」にするか。
