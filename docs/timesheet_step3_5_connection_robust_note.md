# Timesheet Rebuild Step 3.5 connection robust note

## 目的

Step 3.5 の初回スクリプトと修正版スクリプトは、現在の `AppDrawingMode.cpp` の構造差分により失敗した。
この修正版は anonymous namespace の位置に依存せず、`App::drawDrawingMode()` の開始直後へ表示専用 `TimesheetPanel` 呼び出しを追加する。

## 方針

- まだ編集しない。
- 保存しない。
- キャンバス表示に反映しない。
- 再生、PNG、MP4出力に反映しない。
- `TimesheetPanel` の表示確認だけを行う。

## 注意

この段階ではウィンドウを閉じても次フレームで再表示される。
閉じる/再表示の自然な挙動は、App側の正式なパネル表示状態を設ける後続Stepで整える。
