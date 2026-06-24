## 2026-06-25 Phase 1.5-Step 9: FloodFill foundation

### 作業概要
- バケツ塗りツールの土台を追加した。
- ToolKind に FloodFill を追加し、左サイドバーから選択できるようにした。
- Paint レイヤー上でクリックした位置から塗り広げる処理を追加した。

### 変更ファイル
- CMakeLists.txt
- src/brush/BrushSettings.h
- src/fill/FloodFill.h
- src/fill/FloodFill.cpp
- src/ui/App.h
- src/ui/AppInput.cpp
- src/ui/AppDrawingMode.cpp
- src/ui/panels/BrushPanel.cpp

### 実装内容
- src/fill/FloodFill.h/.cpp を新規作成。
- Normal / ColorTrace レイヤーのストロークを壁マスクとして扱う。
- Paint レイヤーには横方向の塗りストローク列として結果を追加する。
- B / E / G ショートカットでブラシ / 消しゴム / バケツを切り替えられる。
- BrushPanel に許容範囲と隙間閉じpxのUIを追加した。

### 未完了
- 塗り結果はまだベクター風の横ストローク表現であり、専用PaintBitmapではない。
- はみ出し防止の縮小処理は未実装。
- 既存Paint色との許容範囲比較は未実装。

### 次にやること
- FloodFillの使い勝手確認後、Paintレイヤー専用の保存形式や塗りキャッシュ方式を検討する。
- または軽量消しゴムプレビューへ進む。

### 判断待ち（私への確認事項）
- バケツ塗り結果を今後もストローク正本として持つか、Paint専用のピクセル/領域データとして分けるか。
