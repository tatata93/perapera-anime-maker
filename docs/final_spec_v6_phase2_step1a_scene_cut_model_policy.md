# final_spec_v6 Phase 2 Step 1-a: Scene / active Cut minimal model

## 作業ツリー

```text
final_spec_v6.md
└─ Phase 2: ファイル構成改定 + セル概念の整理
   ├─ Phase 2-pre: 本格移行前の整理
   │  └─ Step T2: 通常セル + Timesheet 管理方針を実装へ接続する（完了）
   ├─ Phase 2 Step 1: Project -> Scene -> Cut -> Cell 構造の最小導入
   │  ├─ Step 1-a: Scene / active Cut モデルの最小定義（今回）
   │  ├─ Step 1-b: 既存 Project.cells を active Cut 相当として扱うブリッジ
   │  ├─ Step 1-c: 既存UIを壊さず current Scene / current Cut 表示を足す
   │  └─ Step 1-d: Cut 所属 Timesheet と通常 Cell category の関係を確認する
   ├─ Phase 2 Step 2: scenes/scene_NNN/cuts/cut_NNN/cells 保存移行
   ├─ Phase 2 Step 3: Cut単位の Timesheet / Cell / Camera 接続
   └─ Phase 3-pre: 簡易撮影 / セル配置ウィンドウ
```

## 目的

`final_spec_v6.md` の本流である `Project -> Scene -> Cut -> Cell` 構造へ進む前に、
既存 Project 保存/UIを壊さない範囲で Scene と active Cut の最小モデルを定義する。

## 今回の実装

- `src/core/Scene.h` を追加する。
- `perapera::Scene` は `id`, `name`, `memo`, `cuts`, `cutOrder` を持つ。
- `activeCut(scene, index)` は範囲外 index を最後の Cut に丸める。
- `resolvedCutOrder(scene)` は `cutOrder` が空なら `cuts` の id 順を返す。
- `tools/scene_cut_model_selftest.cpp` を追加し、上記の最小動作を検査する。
- `perapera_scene_cut_model_selftest` CMake target を追加する。

## 今回やらないこと

- `Project` 直下の `cells` を移動しない。
- `ProjectIO.cpp` の保存形式を変えない。
- `scenes/scene_NNN/cuts/cut_NNN/cells/` 保存へ移行しない。
- UIにシーン管理パネルを追加しない。
- Scene Plate / シーンパネルを復活させない。
- Timesheet の分離保存方針を変えない。

## 判断

Scene はデータモデルであり、以前削除した Scene Plate / シーンパネルとは別物として扱う。
ユーザーが不要としたのは、背景・レイアウト・BOOKを通常 Cell + Timesheet から切り離す Scene Plate / scene panel 経路である。
Phase 2 の Scene は、Projectを分割管理するための上位データ構造として必要である。
