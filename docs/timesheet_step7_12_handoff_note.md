# Timesheet Rebuild Step 7.12 Handoff Note

## 現状

- `ScenePlate` 最小モデルは `src/core/ScenePlate.h/.cpp` にある。
- `Cut` は `ScenePlateStack scenePlates` を持つ。
- `CutIO` は `scenePlates` を `cut.json` に保存/読み込みする。
- タイムシートの同一セル・同一T重複は、core正規化で後勝ち1件へ整理する。
- `perapera_cut_io_selftest.exe` と `perapera_scene_plate_selftest.exe` で Scene Plate の往復保存と判定を確認できる。

## 次にやる Step 7.12: Scene Plate 表示UI

目的は、画像読み込みやキャンバス描画の前に、Scene Plate 一覧を操作できるUIを作ること。

UIで扱うべき項目:

- 表示ON/OFF: `ScenePlate::visible`
- 参照のみ / プレビューのみ / 出力対象: `ScenePlate::outputMode`
- 種類: `ScenePlate::kind`
- 表示名: `ScenePlate::displayName`
- ロック: `ScenePlate::locked`
- 不透明度: `ScenePlate::opacity`
- 表示順: `ScenePlate::zOrder`
- T範囲: `startTimelineFrame` / `endTimelineFrame`

## 注意

- Scene Plate を通常セルやタイムシート列に混ぜない。
- `ReferenceOnly` は作画参照用。正式出力には含めない。
- `PreviewOnly` はアプリ内プレビュー用。正式PNG/MP4出力には含めない。
- `RenderOutput` は正式出力対象。
- 画像読み込みとキャンバス表示は次段階に分ける。
- 既存の通常プロジェクト保存/読み込みは、まだ `CutIO` と完全統合されていない。UI試作では `Cut` モデルと `CutIO` の仕様を壊さないことを優先する。

## 推奨確認

変更後は最低限これを実行する。

- `cmake --build --preset windows-debug`
- `build/bin/perapera_scene_plate_selftest.exe`
- `build/bin/perapera_cut_io_selftest.exe`
- `build/bin/perapera_timesheet_resolver_selftest.exe`
