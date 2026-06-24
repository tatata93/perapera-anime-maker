# WORK_LOG

## Phase 1.5 Step 14b: MyPaint stroke commit performance guard

### 目的

Phase 1.5 Step 14でMyPaintブラシ設定を反映したあと、1ストローク確定時にUIが止まるほど処理が長くなる問題を抑える。

### 変更内容

- `src/brush/MyPaintBrushEngine.cpp`
  - MyPaintのdab密度を低めに制限。
  - `smudge` / 水彩系の重いget_colorサンプリングを暫定停止。
  - get_colorは透明色を返す軽量実装に変更。
  - 入力点が密すぎる場合はlibmypaintへ渡す点を間引く。
  - 長いストローク・大きいブラシ・過密spacingなど、確定時コストが大きいものはSimple互換へ退避。

### 実装方針

- まず作画中の停止感を消すことを優先。
- MyPaintの描き味は維持しつつ、重すぎるケースだけSimple互換へ退避。
- 水彩/混色の本格挙動は後続StepでCanvasBitmap側の軽量サンプリング設計を行ってから再導入する。

### ビルド

Windows + vcpkg + SDL3環境のため、ChatGPTコンテナ内では実ビルド未実行。
ユーザー環境で完全クリーンのフルビルド確認を行う。

### ランタイム確認項目

- MyPaintBrushEngineで1ストローク確定時に長く固まらない。
- 長い線を描いても確定後に線が残る。
- 短い線、交差線、急カーブが残る。
- 水彩パラメータを変更しても保存/読み込みが壊れない。
- SimpleBrushEngineは従来通り動く。
