# Timesheet Rebuild Step 5: App内正式Timesheet一時保持

## 目的

Step 4でTimesheetPanel内に保持していた一時入力を、App内の正式core `Timesheet` 一時データへ同期する。

## 今回接続したもの

- `App` が `ui::TimesheetPanelState` をメンバーとして保持する。
- `App` が `Timesheet workingTimesheet_` を一時保持する。
- `TimesheetPanelResult::entryChanged` が立ったとき、`buildTimesheetFromPanelState()` で正式Timesheetへ変換する。
- ステータスメッセージに一時Timesheetの記入数を表示する。

## 今回まだ接続しないもの

- Project保存
- cut.json / timesheet.json 保存
- キャンバス表示反映
- 再生反映
- PNG / MP4 出力反映

## 注意

この段階のTimesheetはApp内の一時データであり、アプリ終了で消える。保存接続は後続Stepで行う。
