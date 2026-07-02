# Timesheet Rebuild Step 7: 共通表示/出力解決境界

## 目的

タイムシートTの解決結果を、キャンバス表示と将来のPNG/MP4出力で同じ形で扱えるようにする。

これまでは、キャンバス表示側が `resolveTimesheetCell()` を直接呼び、セルごとの表示Fをその場で決めていた。
Step 7では、Tとセル一覧から「表示するセルと作画F」のリストを作る `TimesheetSceneResolver` を追加し、キャンバス表示側もその境界を使う。

## 追加したもの

- `src/core/TimesheetSceneResolver.h`
- `src/core/TimesheetSceneResolver.cpp`
- `tools/timesheet_scene_resolver_selftest.cpp`

## 変更したもの

- `CMakeLists.txt`
- `src/ui/AppDrawingMode.cpp`

## 方針

- `TimesheetSceneResolver` はUIへ依存しない。
- `TimesheetSceneResolver` はPNG/MP4書き出しへも依存しない。
- 入力は `Timesheet` と `Cell` 参照の配列だけにする。
- 戻り値は「T何番で、どのセルの何番作画Fを表示するか」のリストにする。
- キャンバス表示と出力処理は、今後このリストを共通で使う。

## 今回まだやらないこと

- PNG連番をタイムシートTで書き出すこと。
- MP4をタイムシートTで書き出すこと。
- ExportPanelのUI変更。
- タイムシート再生の音声同期。
- セリフ/カメラ/撮影/素材メモ欄の出力反映。

## 確認ポイント

- `perapera_timesheet_scene_resolver_selftest.exe` が通る。
- タイムシートプレビューのキャンバス表示がStep 6.6と同じ挙動を保つ。
- キャンバス左上の表示に、解決された表示セル数が出る。
- 通常タイムライン再生中は、引き続き作画F再生表示が優先される。

## 次の作業候補

1. Step 7.5: PNGExporterにタイムシート解決済みシーンを書き出す入口を追加する。
2. Step 8: ExportPanelに「作画F連番 / タイムシートT連番」の切替を追加する。
3. Step 8.5: MP4書き出しでタイムシートT連番を使う。
4. Step 9: Project保存/読み込みと `timesheet.json` を自動連動させる。
