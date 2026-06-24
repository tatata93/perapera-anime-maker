
## 2026-06-24 Phase 1.5 Step 11b: FloodFill leak guard stronger
### 作業概要
漏れ防止%が大きな面積判定だけでは効きにくかったため、キャンバス端へ到達した塗りも漏れ扱いにする。

### 変更ファイル
- src/brush/BrushSettings.h
- src/fill/FloodFill.h
- src/fill/FloodFill.cpp
- src/ui/panels/BrushPanel.cpp
- WORK_LOG.md

### 実装内容
- leakGuardPercent の初期値を 45 に変更
- FloodFill 探索中に面積上限を超えたら即中止
- 漏れ防止が有効な時、塗りがキャンバス端へ到達したら中止
- UI説明文をキャンバス端漏れ対応に更新

### 未完了
- 背景全体を塗る専用モードは未実装。必要な時は漏れ防止%を0にする。

### 次にやること
- libmypaint導入スタブ
## 2026-06-25 Phase 1.5-Step 10: Lightweight eraser preview

### 作業概要
- 以前の重い消しゴムプレビュー方式を復活させず、描画ループ中に全ストローク再分割しない軽量プレビューを追加した。
- 消しゴムドラッグ中は、現在なぞっている軌跡だけをキャンバス背景色で上から薄くマスクし、消える予定の範囲をリアルタイムに見せる。

### 変更ファイル
- WORK_LOG.md
- src/ui/AppDrawingMode.cpp

### 実装内容
- drawLightweightEraserPreview() を追加。
- drawCanvasArea() で現在フレーム描画後、消しゴムドラッグ中だけ軽量プレビューを描画。
- previewFrameWithEraser() によるフレーム複製・全ストローク再計算は使わない。
- 既存の消しゴム確定処理 removeIntersectingStrokes() は変更しない。

### 未完了
- 実際の削除結果を完全に再現するプレビューではなく、消しゴム軌跡の視覚マスク表示。
- ストローク単位の削除予定ハイライトは未実装。

### 次にやること
- FloodFill改善、ColorTrace出力時置換、またはlibmypaint導入へ進む。

### 判断待ち（私への確認事項）
- この軽量プレビューの見え方で十分か、さらに「削除予定ストロークだけ赤く光らせる」方向へ進めるか。

## 2026-06-25 Phase 1.5-Step 11: FloodFill improvement

### 作業概要
- バケツ塗りの実用性を上げるため、塗り領域を境界から内側へ縮める「はみ出し防止px」を追加した。
- 閉じていない線をクリックした時に画面全体へ塗りが流れる事故を避けるため、「漏れ防止%」を追加した。

### 変更ファイル
- WORK_LOG.md
- src/brush/BrushSettings.h
- src/fill/FloodFill.h
- src/fill/FloodFill.cpp
- src/ui/AppInput.cpp
- src/ui/panels/BrushPanel.cpp

### 実装内容
- FloodFillSettings に insetPx / leakGuardPercent を追加。
- FloodFill.cpp に insetFilledMask() を追加し、塗り結果を境界から内側へ縮める処理を追加。
- 漏れ防止%を超える巨大な塗りは中止し、閉じ線の作り直しや設定調整を促すメッセージを返す。
- BrushPanel に「はみ出し防止px」「漏れ防止%」を追加。
- AppInput.cpp から新設定を FloodFillSettings へ渡すように変更。

### 未完了
- Paint専用のピクセルデータ化は未実装。
- ColorTrace線を最終出力時に隣接Paint色へ置換する処理は未実装。
- 消しゴムの半径方向削り込みは未修正。現在の消しゴムはベクターストローク分割方式のため、線幅を半分だけ削る用途には向かない。

### 次にやること
- libmypaint導入、ColorTrace出力時置換、または消しゴムのピクセル/輪郭ベース改善を検討する。

### 判断待ち（私への確認事項）
- libmypaint導入を次に行うか、先にColorTrace出力置換や消しゴムの設計修正を行うか。
