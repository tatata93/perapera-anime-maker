# DECISIONS - Phase 1.5 Step 14f

## MyPaint realtime stroke connection

Decision:
- libmypaint の描画は確定時一括処理ではなく、ドラッグ中に beginStroke() -> addPoint() -> endStroke() で逐次処理する。

Reason:
- 確定時に stroke 全点を bakeStroke() すると、長いストロークや太いブラシで UI 停止が発生するため。

Implementation notes:
- App は MyPaintBrushEngine myPaintEngine_ を保持する。
- MyPaint の逐次処理中は isMyPaintStrokeActive_ を true にする。
- MyPaint の確定時は bakeStroke を呼ばず、点列だけ Project 側へ保存する。
- CanvasBitmap へ直接描いたピクセルは markActiveBitmapClean() でキャッシュrevisionだけ同期する。
- 保存・読み込み後の再構築は引き続き軽いSimple互換ベイクで行う。
