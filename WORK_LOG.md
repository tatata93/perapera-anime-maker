# WORK_LOG - Phase 1.5 Step 16

## 目的
保存形式の将来拡張に備えて、主要JSONへ `format_version` を追加する。

## 変更
- `project.json` に `format_version: 1` を追加。
- `cell.json` に `format_version: 1` を追加。
- `frame.json` に `format_version: 1` を追加。
- `layer_NNN.json` に `format_version: 1` を追加。
- 読み込みは既存の `value("key", default)` 方針を維持し、`format_version` による分岐処理は追加しない。

## 実装内容
- `src/io/ProjectIO.cpp` に `kProjectFormatVersion` を追加。
- `projectToJson()` / `cellMetaToJson()` / `frameMetaToJson()` / `toJson(const Layer&)` の出力に `format_version` を追加。

## 未完了
- `scene.json` / `cut.json` / `app_state.json` は未実装のため対象外。
- 新しい `scenes/scene/cut/cells` 階層への移行は Phase 2 前の別Stepで実施する。

## 次にやること
- Phase 1.5 Step 17: 彩色モード追加。
