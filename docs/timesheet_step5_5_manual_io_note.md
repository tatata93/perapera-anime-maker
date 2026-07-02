# Timesheet Rebuild Step 5.5: 一時タイムシート手動保存/読み込み

## 目的

Step 5でApp内に保持した正式 `Timesheet` 一時データを、`timesheet.json` として明示ボタンで保存・読み込みできるようにする。

## 変更範囲

- `src/ui/AppDrawingMode.cpp`
  - `TimesheetIO` を呼び出す。
  - 現在のプロジェクトフォルダ直下に `timesheet.json` を保存する。
  - `一時タイムシート保存` ボタンを追加する。
  - `一時タイムシート読込` ボタンを追加する。

## 保存先

現段階ではCut管理にまだ接続していないため、保存先は暫定的に以下とする。

```text
<projectFolder>/timesheet.json
```

既定値では以下になる。

```text
my_anime_project/timesheet.json
```

## まだ接続しないもの

- Project保存との自動連動
- CutIO / cut.json との統合
- キャンバス表示反映
- 再生反映
- PNG / MP4 出力反映
- セリフ / カメラ / 撮影 / 素材メモ欄の編集

## 確認内容

1. タイムシートに作画、原画、中割、ホールド、空セルを入力する。
2. `一時タイムシート保存` を押す。
3. `my_anime_project/timesheet.json` が作成される。
4. タイムシート入力を変更または消す。
5. `一時タイムシート読込` を押す。
6. 保存した内容が表へ戻る。

## 注意

この保存は試験段階であり、通常のProject保存とはまだ連動しない。
