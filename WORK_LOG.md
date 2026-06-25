## 2026-06-25 Phase 1.5 Step 18: ColorTrace最終出力と白キャンバス背景

### 作業概要
- final_spec_v6 に合わせて、Composite書き出し時だけColorTrace線を近傍のPaint色へ置換する処理を追加。
- 作画・彩色どちらでもキャンバス面を白背景に変更。

### 変更ファイル
- src/io/PngExporter.cpp
- src/ui/AppDrawingMode.cpp

### 実装内容
- PngExporterのCompositeラスタライズ時にPaint参照画像を先に生成。
- ColorTraceレイヤーを合成するとき、ColorTraceピクセルごとに近傍Paint色を探索して置換。
- LineTest / ColorTrace / LineOnly の書き出しモードでは既存挙動を維持。
- AppDrawingModeのキャンバス背景を白に固定。

### 未完了
- 置換半径や色選択方式は最小実装。複雑な境界で不自然な場合は後続で調整する。

### 次にやること
- Step 18のビルド・動作確認。
- 問題なければ Phase 1.5 完了確認へ進む。

### 判断待ち
- なし。

## 2026-06-25 Phase 1.5 Step 18b: PNG書き出し用CanvasBitmapアクセサ修正

### 作業概要
- Step 18のPngExporter.cppが参照するCanvasBitmap::pixelsRgba()がヘッダに存在しない環境でビルドが落ちる問題を修正。

### 変更ファイル
- src/render/CanvasBitmap.h

### 実装内容
- CanvasBitmapにCPU側RGBAピクセルバッファをconst参照で返すpixelsRgba()を追加。
- PngExporter.cppのColorTrace置換処理・MyPaint書き出し処理は変更しない。

### 未完了
- なし。

### 次にやること
- Step 18のフルビルドと書き出し動作確認。

### 判断待ち
- なし。
