# final_spec_v6 Phase 2-pre Step T2-d: Cell category and Timesheet track alignment

## 作業ツリー

```text
final_spec_v6.md
└─ Phase 2: ファイル構成改定 + セル概念の整理
   ├─ Phase 2-pre: 本格移行前の整理
   │  └─ Step T2: 通常セル + Timesheet 管理方針を実装へ接続する
   │     ├─ T2-a: Cell category の正式値を決める（完了）
   │     ├─ T2-b: CellPanel に種別表示・種別変更UIを入れる（完了）
   │     ├─ T2-c: 背景 / レイアウト / BOOK セル作成導線を入れる（完了）
   │     └─ T2-d: Timesheet列と Cell category の関係を崩さない（今回）
   ├─ Phase 2 Step 1: Project -> Scene -> Cut -> Cell 構造の最小導入
   ├─ Phase 2 Step 2: scenes/scene_NNN/cuts/cut_NNN/cells 保存移行
   ├─ Phase 2 Step 3: Cut単位の Timesheet / Cell / Camera 接続
   └─ Phase 3-pre: 簡易撮影 / セル配置ウィンドウ
```

## 目的

背景、レイアウト、BOOK、エフェクト、参考素材を Scene Plate / シーンパネルではなく通常 Cell として扱う。
通常 Cell として扱う以上、category が何であっても Timesheet のセル列から除外しない。

## 決定

- `character`, `background`, `layout`, `book`, `effect`, `reference` はすべて通常 Cell category である。
- Timesheet track は `cellId` をキーにし、category で除外・分岐しない。
- CellPanel はセル管理だけを担当し、Timesheet 編集UIを持たない。
- TimesheetPanel / TimesheetPanelBridge は Timesheet 側の一時UI状態と core Timesheet の変換境界を担当する。
- Phase 2 Step 1 以降で Project -> Scene -> Cut -> Cell に移行しても、この関係は維持する。

## 今回の実装

- `tools/cell_timesheet_alignment_selftest.cpp` を追加する。
- `perapera_cell_timesheet_alignment_selftest` CMake target を追加する。
- selftest では 6種類の category を持つ通常 Cell を作り、同じ順序で Timesheet track を作成できることを確認する。
- `ensureTimesheetTrack` が既存 track を重複作成しないことを確認する。

## 今回やらないこと

- CellPanel から Timesheet列を直接編集すること。
- Timesheet列の自動追加UI。
- Project -> Scene -> Cut -> Cell の保存構造移行。
- 撮影モードのセル配置UI。
- Scene Plate / シーンパネルの復活。

## 後続作業

次は `final_spec_v6 Phase 2 Step 1-a` として、Scene / Cut モデルの最小導入と、既存 Project.cells を active Cut 相当として扱うブリッジを検討する。
