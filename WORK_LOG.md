# WORK_LOG

## Phase 1 Step 1-4 v24: timeline real horizontal scroll

- `src/ui/panels/TimelinePanel.cpp` のフレームボタン列を、実際に横スクロールできる専用Childへ再実装。
- 横スクロールバーに加えて、ホイール操作と `<< < > >>` / `選択へ` ボタンを追加。
- 15フレーム以上でも後続フレームを選択できるようにした。
