# Timesheet Rebuild Step 7.11: Scene Plate 最小モデル

## 目的

絵コンテ、レイアウト、仮背景、完成背景を、作画セル列やタイムシート列へ混ぜずに扱うための最小モデルを追加する。

この段階では、キャンバス表示、画像読み込み、出力にはまだ接続しない。まずデータの意味と安全な正規化を固定する。

追記: 引き渡し前整備で、`CutIO` の `cut.json` 保存/読み込みへ `scenePlates` を接続した。Step 7.12 の表示UIは、作ったプレートが保存で消えない前提で進められる。

## 背景

今後、作画時に以下のような参照をキャンバス背景へ置きたい。

- 絵コンテ
- レイアウト
- 背景原図
- 仮背景
- 完成背景
- 資料画像

これらを通常のセル列に混ぜると、タイムシートのセル管理、作画F、出力対象が混乱する。
そのため `ScenePlate` として別管理する。

## 追加ファイル

- `src/core/ScenePlate.h`
- `src/core/ScenePlate.cpp`
- `tools/scene_plate_selftest.cpp`

## 変更ファイル

- `src/core/Cut.h`
- `CMakeLists.txt`

## Scene Plate の区分

### 種類

- `Storyboard`: 絵コンテ
- `Layout`: レイアウト、背景原図
- `ReferenceImage`: 資料画像
- `TemporaryBackground`: 仮背景
- `FinalBackground`: 完成背景

### 出力モード

- `ReferenceOnly`: 作画参照のみ。PNG/MP4へ出さない。
- `PreviewOnly`: アプリ内プレビュー用。正式出力へ出さない。
- `RenderOutput`: 正式出力対象。

出力可否は種類ではなく `outputMode` で決める。
たとえば `FinalBackground` でも `ReferenceOnly` なら出力しない。

## 持つ情報

- id
- 表示名
- 種類
- 出力モード
- 画像パス
- 表示ON/OFF
- ロック
- 不透明度
- 表示順
- T範囲
- 位置、拡大率、回転

## Cut との関係

`Cut` に `ScenePlateStack scenePlates` を追加する。

`scenePlates` は `cut.json` に保存する。
タイムシート本体は引き続き `timesheet.json` に分離する。

## 今回やらないこと

- 画像読み込みUI
- キャンバス背景表示
- 位置調整UI
- PNG/MP4出力
- タイムシートとの連動

## 確認

`perapera_scene_plate_selftest.exe` が通れば、以下を確認できる。

- 文字列変換
- 不透明度の正規化
- T範囲の正規化
- Scene Plate IDの重複回避
- zOrder順の並び替え
- ReferenceOnly / PreviewOnly / RenderOutput の判定

`perapera_cut_io_selftest.exe` も通れば、以下を確認できる。

- `cut.json` に `scenePlates` が保存される
- `scenePlates` が読み込み時に復元される
- zOrder順に正規化される
- `cut.json` と `timesheet.json` の保存分担が壊れていない
