# final_spec_v6 Phase 2-pre Step T2: Cell category policy

## 作業ツリー

```text
final_spec_v6.md
└─ Phase 2: ファイル構成改定 + セル概念の整理
   ├─ Phase 2-pre: 本格移行前の整理
   │  ├─ Step T1: Scene Plate / シーンパネル系を本流から外す
   │  └─ Step T2: 通常セル + Timesheet 管理方針を実装へ接続する
   │     ├─ T2-a: Cell category の正式値を決める
   │     ├─ T2-b: CellPanel に種別表示・種別変更UIを入れる
   │     ├─ T2-c: 背景 / レイアウト / BOOK セル作成導線を入れる
   │     └─ T2-d: Timesheet列と Cell category の関係を崩さない
```

## 目的

背景、レイアウト、BOOK、エフェクト、参考素材を Scene Plate / シーンパネルではなく通常 Cell として扱う。
通常 Cell として扱うことで、タイミングは Timesheet のセル列で管理できる。

## 正式 category

```text
character
background
layout
book
effect
reference
```

## 今回やること

- CellPanel の category 候補を上記6種類に揃える。
- 既存の Add Cell / Edit Cell の category UIを使い、背景・レイアウト・BOOKを通常セルとして選べる状態にする。
- Scene Plate は復活させない。

## 今回やらないこと

- Project -> Scene -> Cut -> Cell の全面移行。
- `scenes/scene_NNN/cuts/cut_NNN/cells/` 保存への移行。
- Timesheet 印刷。
- 簡易撮影パネル。
- セル配置の移動・拡大・回転UI。

## 後続作業

次は `T2-c` として、背景・レイアウト・BOOK セルを素早く作成するプリセットボタンを検討する。
その後、Phase 2 Step 1 で `Project -> Scene -> Cut -> Cell` の最小導入へ進む。
