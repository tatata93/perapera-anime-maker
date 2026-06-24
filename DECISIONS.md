# DECISIONS

## Phase 1.5 Step 14c

### 決定: MyPaintは確定時一括ではなくドラッグ中逐次処理にする

`bakeStroke(stroke全点)` をペンを離したタイミングで呼ぶと、長い線・太い線・dab密度の高い線でUIが停止する。
この停止は後段の軽量化ガードでは根本解決にならないため、MyPaintの呼び出し単位をストローク全体から入力点単位へ変更した。

### 決定: 保存データ再構築ではSimple互換ベイクを使う

逐次描画中はCanvasBitmapへlibmypaintのdabを直接描く。
一方、保存・読み込み・オニオン・キャッシュ再構築で全ストロークを再描画する場面では、libmypaintを一括実行すると停止が再発する。
そのため、保存済みMyPaintストロークの再構築は当面Simple互換で行う。

### 決定: MyPaint確定時はmarkAllDirtyしない

MyPaintのピクセルはドラッグ中に既にCanvasBitmapへ入っている。
確定時に `markAllDirty()` するとキャッシュが破棄され、直後に保存用点列から再ベイクされて停止原因が復活する。
そのため `markActiveBitmapClean()` でProject側の点列追加とCanvasBitmap revisionだけを同期する。

### 決定: 未確定ストロークのキャンセル時だけmarkAllDirtyする

Escapeキャンセルでは、CanvasBitmapへ直接描いた未保存ピクセルを消す必要がある。
この場合は保存済みストロークから再構築するため `markAllDirty()` を使う。

## Phase 1.5 Step 14d decision

`MyPaintState` は libmypaint ヘッダ依存を `MyPaintBrushEngine.cpp` に閉じ込めるため前方宣言のままにする。
ただし MSVC では `std::unique_ptr<MyPaintState>` の既定 deleter がヘッダ側で不完全型 delete 警告 C4150 を出すため、custom deleter を使う。
これにより `MyPaintState` の実体と delete を `.cpp` 側に閉じ込める。
