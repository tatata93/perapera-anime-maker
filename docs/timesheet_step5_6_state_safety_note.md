# Timesheet Rebuild Step 5.6: タイムシート一時状態の安全正規化

## 目的

Step 5.5 までで、タイムシート表の一時入力を `TimesheetPanelState` に保持し、手動で `timesheet.json` へ保存/読み込みできるようになった。

次にキャンバス表示へ反映する前に、セル追加・セル削除・総フレーム数変更・古い `timesheet.json` 読み込みで不整合が残らないよう、UI一時状態を現在のセル列とカット尺に合わせて正規化する。

## 今回やったこと

- `normalizeTimesheetPanelStateForViewModel()` を追加した。
- 選択中Tを `0 .. totalFrames - 1` にクランプする。
- 選択セル列を現在のセル列範囲へクランプする。
- 作画F番号入力を1以上へクランプする。
- 存在しない `cellId` の一時entryを削除する。
- 範囲外Tの一時entryを削除する。
- `Empty` entryを削除する。
- `Hold / Null` の作画F番号を0へ正規化する。
- 同一 `T x cellId` に複数entryがある場合、後のentryを採用する。
- `AppDrawingMode.cpp` で、描画前・保存前・読み込み後にこの正規化を通すようにした。
- `timesheet_panel_bridge_selftest.cpp` に正規化テストを追加した。

## 今回やらないこと

- キャンバス表示反映。
- 再生反映。
- PNG / MP4 出力反映。
- Project保存との自動連動。
- セリフ / カメラ / 撮影 / 素材メモ欄の編集。

## 確認ポイント

- ビルドが通る。
- `perapera_timesheet_panel_bridge_selftest.exe` が通る。
- タイムシートを入力できる。
- セル追加・セル削除後にタイムシート上で落ちない。
- 保存・読み込み後に赤いDear ImGui警告やクラッシュが出ない。
