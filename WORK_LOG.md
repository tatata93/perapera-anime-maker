# WORK_LOG

## 2026-06-23 Phase 0: リポジトリ初期化

### 作業概要

既存コードを引き継がない完全新規リポジトリとして、最小のC++20 / SDL3 / Dear ImGuiプロジェクトを開始する。

### 変更ファイル

- `.gitignore`
- `.gitattributes`
- `README.md`
- `WORK_LOG.md`
- `DECISIONS.md`
- `CMakeLists.txt`
- `CMakePresets.json`
- `src/app/main.cpp`

### 実装内容

- CMake + Ninja を前提にしたビルド構成を追加
- FetchContentでSDL3、Dear ImGui、nlohmann/jsonを取得する構成を追加
- SDL3ウィンドウとDear ImGuiデモを表示する最小エントリーポイントを追加

### 未完了

- 作画データ構造
- CanvasBitmap
- CanvasRenderer
- 保存・読み込み
- レイヤー、フレーム、セル管理

### 次にやること

Phase 1 Step 1-1として、UIや描画に依存しないcoreデータ構造とJSON保存形式を実装する。

### 判断待ち（私への確認事項）

なし。

---

## 2026-06-23 Phase 1-Step 1-1: データ構造とJSON保存

### 作業概要

core層にProject / Cell / Frame / Layer / Strokeの最小データ構造を追加し、io層に仕様書の保存形式でJSON保存・読み込みを行うProjectIOを追加する。
UI・描画・SDL_Textureには触れず、Phase 1 Step 1-1の責務だけに限定する。

### 変更ファイル

- `CMakeLists.txt`
- `src/core/StrokePoint.h`
- `src/core/Stroke.h`
- `src/core/Stroke.cpp`
- `src/core/Layer.h`
- `src/core/Layer.cpp`
- `src/core/Frame.h`
- `src/core/Frame.cpp`
- `src/core/Cell.h`
- `src/core/Cell.cpp`
- `src/core/Project.h`
- `src/core/Project.cpp`
- `src/core/WorkCanvas.h`
- `src/io/ProjectIO.h`
- `src/io/ProjectIO.cpp`

### 実装内容

- Strokeの点列とbounds計算。
- Layer / Frame / Cell / Projectのデフォルト生成と安全な参照補助。
- ProjectIO::saveで`project.json`、`cell.json`、`frame.json`、`layer_NNN.json`を書き出し。
- ProjectIO::loadで`project.json`と`layer_NNN.json`からProjectを復元。
- `composite.png`は仕様どおり保存対象外。

### 未完了

- UIから保存・読み込みを呼ぶ処理は未実装。
- CanvasBitmapや描画への接続は未実装。
- ProjectIOの実ファイル往復テストは次以降で追加予定。

### 次にやること

- Phase 1 Step 1-2: CanvasBitmapとSimpleBrushEngineの追加。
- または、先にProjectIOの小さな動作確認用コード/テストを追加する。

### 判断待ち（私への確認事項）

なし。

## 2026-06-23 Phase 1-Step 1-2: CanvasBitmapとSimpleBrushEngine

### 作業概要

render層に1レイヤー分のCanvasBitmapを追加し、brush層にBrushEngine抽象インターフェースとPhase 1用のSimpleBrushEngineを追加する。
UIやCanvasRendererにはまだ接続せず、ペン確定時だけピクセルへ焼く土台を作る。

### 変更ファイル

- `CMakeLists.txt`
- `src/render/CanvasBitmap.h`
- `src/render/CanvasBitmap.cpp`
- `src/brush/BrushEngine.h`
- `src/brush/SimpleBrushEngine.h`
- `src/brush/SimpleBrushEngine.cpp`

### 実装内容

- CanvasBitmapにRGBA8ピクセルバッファとSDL_Texture管理を追加。
- ストロークをradius*0.5間隔で補間し、円スタンプでピクセルに焼く処理を追加。
- dirty矩形だけSDL_Textureへアップロードする処理を追加。
- 円形消しゴムと全クリア処理を追加。
- BrushEngine抽象インターフェースを追加。
- SimpleBrushEngineを追加し、CanvasBitmapへの焼き込み・消しゴムを橋渡し。

### 未完了

- CanvasRendererとの接続は未実装。
- Appのペン入力との接続は未実装。
- 画面上で線を引くUIはまだ未実装。

### 次にやること

Phase 1 Step 1-3として、CanvasRenderer、CanvasView、2Dカメラの土台を追加する。

### 判断待ち（私への確認事項）

なし。
