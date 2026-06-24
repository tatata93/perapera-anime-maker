## 2026-06-24 Phase 1.5 Step 6: カラーパレット土台
### 作業概要
Phase 1.5の彩色基礎として、作画モード左サイドバーにColorPanelを追加した。
ブラシ色、スウォッチ、最近使った色、色トレス用の基本色をUIから扱えるようにした。

### 変更ファイル
- CMakeLists.txt
- WORK_LOG.md
- src/ui/App.h
- src/ui/AppDrawingMode.cpp
- src/ui/panels/ColorPanel.h
- src/ui/panels/ColorPanel.cpp

### 実装内容
- ColorPanelState / ColorSwatch を追加
- デフォルトスウォッチ（主線、白、赤/青/黄緑トレス、影指定）を追加
- 現在色のColorEdit4を追加
- 現在色をスウォッチへ追加できるUIを追加
- スウォッチクリックでbrushSettings_.colorへ反映
- 最近使った色の履歴を追加

### 未完了
- palettes/palette.jsonへの保存/読み込み
- スウォッチのドラッグ並べ替え
- キャンバススポイト

### 次にやること
- バケツ塗りへ進む前に、ColorPanelのpalette.json保存を入れる
- または、再生中FPS低下対策として指パラ再生の軽量化を入れる

### 判断待ち（私への確認事項）
- 次を「パレット保存」か「指パラ再生軽量化」のどちらにするか
