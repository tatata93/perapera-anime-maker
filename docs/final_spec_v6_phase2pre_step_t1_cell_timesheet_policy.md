# final_spec_v6 Phase 2-pre Step T1: Cell + Timesheet policy for background/layout/BOOK

## 作業位置

- 最上位仕様書: `final_spec_v6.md`
- フェーズ: Phase 2 前整理
- 作業名: Step T1 derived work
- 目的: 背景、レイアウト、BOOK、参考画像、仮背景、完成背景を Scene Plate / シーンパネルではなく、通常セル系と Timesheet 管理へ寄せる。

## 決定

作画モードでは、背景やレイアウトも通常セルと同じ流れで扱う。
タイミングは Timesheet で管理する。

Scene Plate / シーンパネルは、同じ対象を別概念で管理してしまうため、Phase 2 前整理として本流から外す。

## 扱い

- 背景: background category の Cell として扱う。
- レイアウト: layout category の Cell または Layout layer として扱う。
- BOOK: book category の Cell として扱う。
- エフェクト: effect category の Cell として扱う。
- 参考画像: reference category の Cell または作画補助表示として扱う。

## Timesheet

Timesheet は概念上 Cut に属する。
ただし将来的に単体表示・単体印刷・単体編集を行うため、物理保存は `timesheet.json` のように `cut.json` から分離してよい。

## 撮影モード

セルの移動、拡大縮小、回転、透明度、Z順はどのセルにも発生する。
したがって Scene Plate 専用UIではなく、撮影モードの簡易撮影/セル配置UIとして扱う。

## 今回やらないこと

- Project -> Scene -> Cut -> Cell の本格移行
- Cell category の正式追加
- Shooting mode の簡易撮影UI追加
- Timesheet 印刷
- PNG/MP4 出力の最終統合
