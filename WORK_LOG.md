# WORK_LOG

## 2026-06-23 Phase 0: リポジトリ初期化

### 作業概要
完全新規リポジトリ用に、C++20 / CMake / SDL3 / Dear ImGui の最小アプリ骨格を作成した。

### 変更ファイル
- `.gitignore`
- `.gitattributes`
- `README.md`
- `WORK_LOG.md`
- `DECISIONS.md`
- `CMakeLists.txt`
- `CMakePresets.json`
- `src/app/main.cpp`
- `src/ui/Theme.h`

### 実装内容
- SDL3ウィンドウ作成
- SDL3 Renderer作成
- Dear ImGui初期化
- ImGuiデモウィンドウ表示
- ①〜⑤のモードタブ骨格
- 各モード用の空パネル
- ダークテーマ定数の定義と適用

### 未完了
- 作画データ構造
- CanvasBitmap
- レイヤー / フレーム / タイムライン
- 保存 / 読み込み
- PNG / MP4書き出し

### 次にやること
Phase 1 Step 1-1: データ構造とJSON保存を実装する。

### 判断待ち（私への確認事項）
なし。


## 2026-06-23 Phase 0: ImGui通常版ビルド修正

### 作業概要
通常版 Dear ImGui v1.92.8 では `ImGui::SetNextWindowViewport` が使えないため、Phase 0 のメインレイアウト初期化から該当呼び出しを削除した。

### 変更ファイル
- `src/app/main.cpp`
- `WORK_LOG.md`

### 実装内容
- `ImGui::SetNextWindowViewport(viewport->ID)` を削除
- メインウィンドウの位置とサイズは `ImGui::SetNextWindowPos` と `ImGui::SetNextWindowSize` で維持

### 未完了
- なし

### 次にやること
- Phase 0 のビルドと起動確認

### 判断待ち（私への確認事項）
- なし
