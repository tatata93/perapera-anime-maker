
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


## 2026-06-25 Phase 1.5-Step 11c: FloodFill leak guard adjustment

### 作業概要
- シード点やキャンバス端到達だけで漏れ扱いしないようにし、背景塗りや端付近の小さい図形を塗れるようにした。

### 変更ファイル
- src/fill/FloodFill.cpp

### 実装内容
- 漏れ防止の中止条件を面積超過のみに整理。
- queue.size() の上限判定を >= から > に調整。

### 未完了
- バケツ塗りのPaint専用ピクセルデータ化は未実装。

### 次にやること
- libmypaint導入スタブへ進む。

## 2026-06-25 Phase 1.5-Step 12: libmypaint導入スタブ

### 作業概要
- libmypaintを使うMyPaintBrushEngineのビルド経路とスタブを追加した。
- 既存のSimpleBrushEngineを壊さず、BrushPanelからSimple/MyPaintを切り替えられる土台を作った。

### 変更ファイル
- WORK_LOG.md
- DECISIONS.md
- CMakeLists.txt
- src/brush/BrushSettings.h
- src/brush/MyPaintBrushEngine.h
- src/brush/MyPaintBrushEngine.cpp
- src/ui/AppDrawingMode.cpp
- src/ui/panels/BrushPanel.cpp

### 実装内容
- CMakeLists.txt に find_package(libmypaint CONFIG QUIET) を追加。
- libmypaintが無い環境でもビルドが通るように、MyPaintBrushEngineはSimple互換フォールバックにした。
- libmypaintが検出された場合は PERAPERA_HAS_LIBMYPAINT を定義し、利用可能状態をUIに表示する。
- BrushPanelにブラシエンジン切り替えUIを追加。

### 未完了
- libmypaintのdraw_dabコールバック接続は未実装。
- MyPaintのブラシ設定ファイル読み込みは未実装。

### 次にやること
- Step 13でMyPaintBrushEngineの実描画接続を行う。

### 判断待ち（私への確認事項）
- Step 13でlibmypaintの実API接続へ進むか、先にvcpkg環境の確認を行うか。

## 2026-06-25 Phase 1.5 Step 13: MyPaintBrushEngine 実接続
### 作業概要
- libmypaint の `MyPaintSurface::draw_dab` を `CanvasBitmap::paintDab` へ接続した。
- MyPaintBrushEngine 選択時のストロークは `brushEngine: "MyPaint"` として保存する。
- 既存の SimpleBrushEngine と旧プロジェクトの挙動は維持する。

### 変更ファイル
- CMakeLists.txt
- WORK_LOG.md
- DECISIONS.md
- src/core/Stroke.h
- src/core/Stroke.cpp
- src/io/ProjectIO.cpp
- src/render/CanvasBitmap.h
- src/render/CanvasBitmap.cpp
- src/render/CanvasRenderer.h
- src/render/CanvasRenderer.cpp
- src/brush/MyPaintBrushEngine.h
- src/brush/MyPaintBrushEngine.cpp
- src/ui/AppInput.cpp
- src/ui/AppDrawingMode.cpp
- src/ui/panels/BrushPanel.cpp

### 実装内容
- `StrokeBrushEngine` を追加し、ストローク単位で Simple / MyPaint を保持。
- `layer_NNN.json` に `brushEngine` を保存・読み込み。
- `CanvasBitmap::paintDab()` と `sampleAverageColor()` を追加。
- libmypaint の surface コールバックを `CanvasBitmap` へ接続。
- CMake は CONFIG 検出と pkg-config 検出の両方に対応。

### 未完了
- MyPaint の `.myb` ブラシプリセット読み込み。
- 水彩・にじみ・色混ぜパラメータの詳細接続。
- タブレット筆圧の実入力取得。

### 次にやること
- MyPaint 設定値のUI反映を増やす、または ColorTrace 出力時置換に進む。

### 判断待ち（私への確認事項）
- なし。

## 2026-06-25 Phase 1.5 Step 13b: libmypaint検出修正

### 作業概要
- vcpkg manifest modeでlibmypaintはインストール済みなのに、CMake側でPERAPERA_HAS_LIBMYPAINTが定義されない問題を修正。
- CMake config / pkg-config に加えて、`vcpkg_installed/<triplet>` 直下のヘッダ・import libを直接検出するフォールバックを追加。

### 変更ファイル
- CMakeLists.txt

### 実装内容
- `vcpkg_installed/x64-windows/include/libmypaint/mypaint-brush.h` と `vcpkg_installed/x64-windows/lib/mypaint.lib` を直接検出。
- 検出できた場合は `PERAPERA_HAS_LIBMYPAINT=1` を定義。
- libmypaint / GLib系ヘッダのincludeディレクトリを追加。
- `mypaint.lib` を直接リンク。
- `vcpkg_installed/x64-windows/bin` のDLLをexe横へコピーするpost-build処理を追加。

### 未完了
- libmypaint APIの描き味調整は次Stepで継続。

### 次にやること
- MyPaintBrushEngine選択時に「libmypaint未検出」が消えるか確認。
- 線が描けるか確認。

## 2026-06-25 Phase 1.5 Step 13c: MyPaint描画消失フォールバック

### 作業概要
- libmypaint検出後、MyPaintBrushEngine選択時にドラッグ中のプレビューは見えるが、確定後に線が消える問題を修正。
- libmypaintがdabを1つも返さない短いストローク・環境差・設定差の場合、既存Simple焼き込みへ退避する。

### 変更ファイル
- WORK_LOG.md
- DECISIONS.md
- src/brush/MyPaintBrushEngine.cpp

### 実装内容
- MyPaintSurface側でdab呼び出し数と表示可能dab数を数えるようにした。
- 1点だけの短いストロークはSimple焼き込みへ退避する。
- libmypaint描画後に表示可能dabが0個なら、保存形式はMyPaintのままCanvasBitmapへの焼き込みだけSimpleへフォールバックする。

### 未完了
- MyPaint設定値の本格調整、.mybプリセット読み込み、水彩系パラメータは未実装。
- 今回は「線が消えない」ことを優先した安全退避であり、MyPaintらしい描き味調整は次工程。

### 次にやること
- MyPaintがdabを出しているかを画面上で確認し、必要ならStep 13dで設定値・stroke_to呼び出しを調整する。


## 2026-06-25 Phase 1.5 Step 13d: MyPaint交差欠け対策

### 作業概要
- MyPaintBrushEngine選択時、線は残るが交差箇所で遮られたように欠ける問題への安全対策を追加した。

### 変更ファイル
- WORK_LOG.md
- DECISIONS.md
- src/brush/MyPaintBrushEngine.cpp

### 実装内容
- libmypaintのdraw_dabから渡るalpha_eraserを作画ブラシでは無視し、意図しない円形消去を起こさないようにした。
- smudge/colorize/lock_alpha系の基本値を0へ寄せた。
- libmypaintが有効dabを返した場合でも、ストローク確定時だけ細いSimple連続芯を重ね、交差部分や急カーブの欠けを防ぐようにした。

### 未完了
- MyPaintらしい描き味の本格調整、.mybプリセット読み込み、タブレット筆圧入力は未実装。

### 次にやること
- 交差欠けが消えたか確認し、問題なければMyPaint設定UI反映を進める。

## Phase 1.5 Step 14: MyPaint brush settings mapping

### Summary
- Reflected BrushPanel settings into persisted Stroke data.
- Connected opacity / hardness / spacing / pressure mapping / watercolor-like parameters to MyPaintBrushEngine.
- Fixed brush opacity so committed strokes preserve the UI opacity after cache rebuild and save/load.
- Kept SimpleBrushEngine as fallback and kept the Step 13d continuity core, but reduced it so MyPaint dab output remains visible.

### Changed files
- src/core/Stroke.h
- src/io/ProjectIO.cpp
- src/render/CanvasRenderer.cpp
- src/brush/BrushSettings.h
- src/brush/MyPaintBrushEngine.cpp
- src/ui/AppInput.cpp
- src/ui/AppDrawingMode.cpp
- src/ui/panels/BrushPanel.cpp

### Runtime verification
- Full clean build with vcpkg toolchain.
- Select MyPaintBrushEngine and compare Pen/Pencil/Watercolor/Airbrush presets.
- Confirm opacity and hardness changes remain after frame switch, save/load, and restart.
- Confirm SimpleBrushEngine still draws normally.
