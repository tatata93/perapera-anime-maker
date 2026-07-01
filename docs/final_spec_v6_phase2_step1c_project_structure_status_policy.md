# final_spec_v6 Phase 2 Step 1-c: Project structure status UI

## 作業ツリー

```text
final_spec_v6.md
└─ Phase 2: ファイル構成改定 + セル概念の整理
   ├─ Phase 2-pre: 通常セル + Timesheet 管理方針を実装へ接続する（完了）
   ├─ Phase 2 Step 1: Project -> Scene -> Cut -> Cell 構造の最小導入
   │  ├─ Step 1-a: Scene / active Cut モデルの最小定義（完了）
   │  ├─ Step 1-b: 既存 Project.cells を active Cut 相当として扱うブリッジ（完了）
   │  ├─ Step 1-c: 既存UIを壊さず current Scene / current Cut 表示を足す（今回）
   │  └─ Step 1-d: Cut 所属 Timesheet と通常 Cell category の関係確認
   ├─ Phase 2 Step 2: scenes/scene_NNN/cuts/cut_NNN/cells 保存移行
   ├─ Phase 2 Step 3: Cut単位の Timesheet / Cell / Camera 接続
   └─ Phase 3-pre: 簡易撮影 / セル配置ウィンドウ
```

## 目的

Phase 2 の本流は `Project -> Scene -> Cut -> Cell` へ移行することだが、現時点では Project 保存形式を変えない。
この段階では、ユーザーが現在の作業位置を見失わないように、既存 UI 内に小さな Project / Scene / Cut ステータスを表示する。

## 判断

- Scene / Cut は現時点では互換表示として `scene_001` / `cut_001` を表示する。
- 既存 `Project.cells` は Step 1-b の active Cut bridge 経由で active Cut cells として扱う。
- Scene 管理パネルは作らない。
- Scene Plate / シーンパネルは復活させない。
- `AppDrawingMode.cpp` は 800 行を大きく超えるため、新しい表示処理は `ProjectStructureStatusPanel` へ分割する。
- `AppDrawingMode.cpp` には include と呼び出しのみを最小追加する。

## 今回やらないこと

- Project 保存形式変更。
- `Project.h` への `scenes` 追加。
- Scene 管理パネル追加。
- Cut選択UI追加。
- scenes/scene_NNN/cuts/cut_NNN/cells 保存移行。
- 簡易撮影 / セル配置UI。
