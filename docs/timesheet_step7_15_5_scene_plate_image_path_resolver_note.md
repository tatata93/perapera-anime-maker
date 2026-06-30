# Timesheet Rebuild Step 7.15.5: Scene Plate image path resolver unification

## 目的

PowerShell では存在する画像が、Scene Plate UI / image cache 側で `画像未検出` / `image file not found` になる問題を潰す。

対象例:

```text
C:\Users\tak01\Desktop\P1326073.JPG
```

## 変更

- Scene Plate 画像パスの正規化、絶対/相対判定、解決後パス、存在確認を `ScenePlateImageCache` 側の共通関数へ集約した。
- UI表示と `ScenePlateImageCache::textureFor()` が、同じ最終解決パスを見るようにした。
- Windows絶対パスは `projectFolder` と結合しない。
- 相対パスだけ `projectFolder` と結合する。
- UIへ以下の診断表示を追加した。
  - 入力パス
  - 正規化後
  - 絶対パス判定
  - projectFolder
  - 解決パス
  - Win32確認
  - キャッシュ読込パス
  - 読込結果
- `perapera_scene_plate_image_path_selftest` を追加した。

## 確認できること

`perapera_scene_plate_image_path_selftest.exe` で以下を確認する。

- Windows絶対パスが絶対パスとして扱われる。
- 絶対パスが `projectFolder` と結合されない。
- 引用符、CR/LF付きパスが正規化される。
- 相対パスだけ `projectFolder` と結合される。
- `.JPG` のような大文字拡張子も対応候補として扱われる。
- 全角円記号パスも通常の区切りへ正規化される。

## 次の確認

アプリ上で `C:\Users\tak01\Desktop\P1326073.JPG` を指定し、Scene Plate管理UIの表示が以下に近いことを確認する。

```text
絶対パス判定: YES
Win32確認: 入力=OK / 解決=OK
キャッシュ読込パス: C:\Users\tak01\Desktop\P1326073.JPG
読込: 実画像表示OK
```

ここで `Win32確認` はOKなのに `読込` が失敗する場合は、次はWICデコード側の問題として扱う。

## 今回やらないこと

- ファイル選択ダイアログ
- 素材ファイルのプロジェクト内コピー
- Project保存/読み込み統合
- PNG/MP4出力へのScene Plate合成
