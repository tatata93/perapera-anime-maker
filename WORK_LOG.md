# WORK_LOG

## 2026-06-23 Phase 0 Fix: 日本語フォント安定化

### 作業概要
- Windows標準フォントを使い、日本語UIが表示される状態にした。
- Dear ImGuiのフォントアトラスを巨大化させないため、Phase 0で実際に使う文字だけを登録した。

### 変更ファイル
- src/app/main.cpp
- WORK_LOG.md

### 実装内容
- `C:/Windows/Fonts/msgothic.ttc` などのWindows標準日本語フォントを検索。
- 起動失敗を避けるため、最小限の日本語グリフだけを読み込み。
- フォント読み込み状態を画面上に表示。

### 未完了
- フォント選択UIは未実装。

### 次にやること
- Phase 1 Step 1-1 のデータ構造とJSON保存へ進む。

### 判断待ち（私への確認事項）
- なし。

## 2026-06-23 Phase 1 Step 1-1: データ構造とJSON保存

### 作業概要
- 仕様書に従い、作画ソフトの中核データ構造を追加した。
- `Project / Cell / Frame / Layer / Stroke / StrokePoint` をUIや描画から独立したcore層として実装した。
- `ProjectIO` を追加し、仕様書のフォルダ構成でJSON保存・読み込みできるようにした。
- Phase 1開始時の修正として、モードタブに「4 撮影」を追加し、ImGuiデモの初期表示をOFFにした。

### 変更ファイル
- CMakeLists.txt
- WORK_LOG.md
- src/app/main.cpp
- src/core/StrokePoint.h
- src/core/Stroke.h
- src/core/Stroke.cpp
- src/core/Layer.h
- src/core/Layer.cpp
- src/core/Frame.h
- src/core/Frame.cpp
- src/core/Cell.h
- src/core/Cell.cpp
- src/core/WorkCanvas.h
- src/core/Project.h
- src/core/Project.cpp
- src/io/ProjectIO.h
- src/io/ProjectIO.cpp

### 実装内容
- `Stroke::bounds()` でストロークの外接矩形を計算。
- `Layer / Frame / Cell / Project` に仕様書の保存形式と対応するフィールドを追加。
- `Project::createDefault()` で最小プロジェクトを作れるようにした。
- `ProjectIO::save()` で以下を保存。
  - `project.json`
  - `cells/cell_001/cell.json`
  - `cells/cell_001/frames/frame_001/frame.json`
  - `cells/cell_001/frames/frame_001/layer_001.json`
- `ProjectIO::load()` で上記JSONからデータ構造を復元。
- `composite.png` は保存しない。点列JSONを正とする。
- モードタブを `Project / Storyboard / Previs / Drawing / Shooting / Export` の6つに更新し、「④ 撮影」を追加。
- `showImguiDemo` の初期値を `false` に変更。

### 未完了
- 保存・読み込みUIへの接続は未実装。
- JSON保存の画面操作確認はまだできない。
- 描画、CanvasBitmap、ブラシエンジンは未実装。

### 次にやること
- Phase 1 Step 1-2: CanvasBitmap と SimpleBrushEngine を追加する。

### 判断待ち（私への確認事項）
- なし。
