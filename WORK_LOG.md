## 2026-06-26 Phase 1.5 Step 18c: ColorTrace出力の色残りと白隙間修正
### 作業概要
Composite PNG/MP4書き出しでColorTrace線の赤・青・黄緑が残る問題と、Paint塗りと線の間に白い隙間が残る問題を修正した。

### 変更ファイル
- src/io/PngExporter.cpp
- src/render/CanvasBitmap.h

### 実装内容
- Composite出力ではColorTraceレイヤーをそのまま合成しないようにした。
- ColorTrace線のアルファをマスクとして扱い、近傍のPaint色で吸収する処理に変更した。
- Normal線の周囲に残る白い隙間を、小さい半径で近傍Paint色に置換する処理を追加した。
- ColorTrace線の周囲に残る白い隙間を、少し広めに近傍Paint色へ置換する処理を追加した。
- 黒いNormal線そのものはPaint色で潰さないよう、暗い線画ピクセルを保護した。
- Step18bのCanvasBitmap::pixelsRgba()アクセサも同梱し、PNG書き出し側がCPU側RGBAを読める状態を維持した。

### 未完了
- なし。

### 次にやること
- Composite PNG/MP4でColorTrace線が残らないこと、線とPaintの間の白い隙間が消えることを確認する。
- 出力が安定したらPhase 2前のファイル構成改定に進む。

### 判断待ち
- ColorTrace線を完全に消すのではなく、Paint色が見つからない場合も何らかの警告色で残すべきかは必要になった時点で判断する。
