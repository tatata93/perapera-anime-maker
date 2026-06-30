# Timesheet Rebuild Step 7.15: Scene Plate 実画像キャンバス表示

## 目的
Scene Plate に指定した画像パスを実際に読み込み、キャンバス背景へ表示する。
絵コンテ、レイアウト、参照画像、仮背景、完成背景をセル列やタイムシート列へ混ぜず、作画の下敷きとして見られる状態へ進める。

## 実装内容

- `src/ui/ScenePlateImageCache.h/.cpp` を追加した。
- `App` に Scene Plate 画像用の SDL_Texture キャッシュを追加した。
- Windows では WIC を使って PNG / JPEG / BMP などを RGBA に変換し、SDL_Texture 化する。
- 読み込みに成功した Scene Plate はキャンバス上で実画像として表示する。
- 読み込みに失敗した Scene Plate は従来どおりダミー矩形表示へフォールバックする。
- `visible` / `opacity` / `zOrder` / `T範囲` / `位置` / `拡大率` / `回転` を実画像表示にも反映する。
- renderer が変わった場合は Scene Plate 画像キャッシュを破棄する。

## まだやらないこと

- ファイル選択ダイアログ
- 画像をプロジェクト内へコピーする素材管理
- Scene Plate の変形ハンドル
- PNG/MP4 出力への Scene Plate 合成
- 通常 Project 保存/読込との正式統合

## 注意

`.webp` は画像パス候補として受け付けるが、Windows/WIC 側でデコーダが利用できない環境では読み込み失敗になり、ダミー矩形へ戻る。
この挙動はクラッシュ回避を優先したフォールバックである。
