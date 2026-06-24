## 2026-06-24 Phase 1.5 Step 8: palette.json 保存・読み込み

### 作業概要
ColorPanel のスウォッチと最近使った色を `palettes/palette.json` に保存・読み込みできるようにした。
プロジェクト保存・読み込みの流れに追加し、描画データとは別責務の `AppPaletteIO.cpp` に分離した。

### 変更ファイル
- `CMakeLists.txt`
- `WORK_LOG.md`
- `src/ui/App.h`
- `src/ui/AppDrawingMode.cpp`
- `src/ui/AppProjectIO.cpp`
- `src/ui/AppPaletteIO.cpp`
- `src/ui/panels/ColorPanel.h`
- `src/ui/panels/ColorPanel.cpp`

### 実装内容
- `src/ui/AppPaletteIO.cpp` を追加
- `saveColorPalette()` / `loadColorPalette()` を追加
- `palettes/palette.json` に以下を保存
  - `brushColor`
  - `editColor`
  - `selectedSwatchIndex`
  - `swatches`
  - `recentColors`
- プロジェクト保存時に palette.json も保存
- プロジェクト読み込み時に palette.json を復元
- palette.json が存在しない旧プロジェクトはデフォルトパレットで開く
- ColorPanel に保存状態の表示と「デフォルトに戻す」ボタンを追加

### 未完了
- スウォッチ並べ替え
- キャンバススポイト
- palette.json の複数パレット管理

### 次にやること
- FloodFill（バケツ塗り）の土台
- または軽量消しゴムプレビュー

### 判断待ち（私への確認事項）
- 次にバケツ塗りへ進むか、先に消しゴムプレビューを入れるか。
