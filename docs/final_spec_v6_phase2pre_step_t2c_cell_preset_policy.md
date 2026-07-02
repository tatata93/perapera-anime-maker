# final_spec_v6 Phase 2-pre Step T2-c: Cell preset creation policy

## 作業ツリー

```text
final_spec_v6.md
└─ Phase 2: ファイル構成改定 + セル概念の整理
   ├─ Phase 2-pre: 本格移行前の整理
   │  └─ Step T2: 通常セル + Timesheet 管理方針を実装へ接続する
   │     ├─ T2-a: Cell category の正式値を決める
   │     ├─ T2-b: CellPanel に種別表示・種別変更UIを入れる
   │     ├─ T2-c: 背景 / レイアウト / BOOK セル作成導線を入れる
   │     └─ T2-d: Timesheet列と Cell category の関係を崩さない
   ├─ Phase 2 Step 1: Project -> Scene -> Cut -> Cell 構造の最小導入
   ├─ Phase 2 Step 2: scenes/scene_NNN/cuts/cut_NNN/cells 保存移行
   └─ Phase 3-pre: 簡易撮影 / セル配置ウィンドウ
```

## 目的

背景、レイアウト、BOOK、エフェクト、参考素材を Scene Plate / シーンパネルではなく通常 Cell として作成できるようにする。
通常 Cell として作ることで、後続の Timesheet列管理に接続しやすくする。

## 今回追加する導線

CellPanel の通常セル管理UIに、以下のプリセット作成ボタンを追加する。

```text
+ Char      -> category = character
+ BG        -> category = background
+ Layout    -> category = layout
+ BOOK      -> category = book
+ FX        -> category = effect
+ Ref       -> category = reference
```

## 仕様上の扱い

- これらはすべて通常 Cell である。
- 背景・レイアウト・BOOK も Timesheet のセル列でタイミングを管理する。
- Scene Plate / シーンパネルは復活させない。
- 移動、拡大縮小、回転、透明度、Z順は後続の撮影モード / 簡易撮影UIで扱う。

## 今回やらないこと

- Timesheet列への自動追加・同期。
- Project -> Scene -> Cut -> Cell の全面移行。
- `scenes/scene_NNN/cuts/cut_NNN/cells/` 保存への移行。
- 簡易撮影パネル。
- セル配置の移動・拡大・回転UI。
- Scene Plate の復活。

## 後続作業

次は Step T2-d として、Cell category と Timesheet列の関係を壊さない整理を行う。
その後、Phase 2 Step 1 として Project -> Scene -> Cut -> Cell の最小導入へ進む。
