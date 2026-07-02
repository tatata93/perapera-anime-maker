# Timesheet Rebuild Step 4.5: Panel/Core Bridge Note

## 目的

Step 4で追加したタイムシートUIの一時入力を、正式な `core::Timesheet` モデルへ変換する境界を追加する。
この段階では、保存・キャンバス表示・再生・PNG/MP4出力には接続しない。

## 追加内容

- `src/ui/panels/TimesheetPanelBridge.h`
- `src/ui/panels/TimesheetPanelBridge.cpp`
- `tools/timesheet_panel_bridge_selftest.cpp`

## 方針

`TimesheetPanelState::entries` はUI上の一時編集状態であり、直接保存データにしない。
`buildTimesheetFromPanelState()` で正式 `Timesheet` に変換する。

変換時のルール:

- UI側Tは0始まり
- core側Tは1始まり
- `Empty` は未記入なので正式Timesheetには保存しない
- `Hold` は正式 `TimesheetEntryType::Hold` として保存する
- `Null` は正式 `TimesheetEntryType::Null` として保存する
- `Drawing / Key / Inbetween` は作画F番号を1以上に正規化する
- ビュー上に存在しないセルIDの一時入力は保存対象にしない

## 今回まだしないこと

- AppのProject/CutへTimesheetを保持させる
- `timesheet.json` への保存接続
- キャンバス表示反映
- 再生・書き出し反映

## 次の推奨

Step 5では、まだキャンバス反映に行かず、`TimesheetPanelState` と `Timesheet` をApp内の一時状態として保持し、保存・読込の入口を作るのがよい。
