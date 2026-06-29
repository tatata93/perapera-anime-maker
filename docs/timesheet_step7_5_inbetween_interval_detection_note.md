# Timesheet Rebuild Step 7.5: 原画間検出

## 目的

「原画を先に作る → その間に中割を作る」という作画手順を、タイムシート上で自然に扱えるようにする。

このStepでは中割を自動配置しない。選択中のTとセルから、前原画、次原画、その間の中割、選択Tの位置を検出して表示する。

## 実装内容

- `src/core/TimesheetInbetweenResolver.h`
- `src/core/TimesheetInbetweenResolver.cpp`
- `tools/timesheet_inbetween_resolver_selftest.cpp`
- `AppDrawingMode.cpp` に「原画間検出」確認UIを追加

## 用語

- `T`: タイムシート上の時間位置
- `作画F`: 実際に描かれた絵の番号
- `原画`: 原画として指定された作画F
- `中割`: 前後原画の間に置かれた作画F
- `コマ数`: 表示の持続時間
- `フレーム`: 内部処理/出力側の用語

## 今回やらないこと

- 中割作画Fの自動作成
- タイムシートへの中割自動配置
- ライトテーブルの自動切替
- 絵コンテ/背景参照プレートの実装
- PNG/MP4出力変更

## 背景/絵コンテ対応方針

絵コンテ、レイアウト、背景原図、仮背景、完成背景はタイムシートセル列には混ぜない。
将来は `Scene Plate` として別管理し、ReferenceOnly / PreviewOnly / RenderOutput の用途を分ける。
