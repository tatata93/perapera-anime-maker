# perapera-anime-maker software specification and AI work instruction v6.0

This document is the top-level specification plus implementation instruction document for perapera-anime-maker. The historical filename is `final_spec_v6.md`, but the intended role is: spec, work order, and handoff guide for humans and AI agents.

See also: `docs/perapera_anime_maker_spec_and_ai_work_instruction_v6.md`.

---
# ぺらぺらアニメ作り機 最終仕様書 v6.0
# 完全新規作成版 — ChatGPT向け実装指示書
# 現在の実装: Phase 1.5 進行中
# v5.0からの矛盾・問題点を全て修正した版

---

## 0. このドキュメントの使い方

**役割の分担**
- 実装者: ChatGPT
- 仕様決定者: 私（人間）

**必須ルール**
- 大きな設計変更・ライブラリ追加・方針が複数ある場合は必ず候補を出して私に確認する
- 小さなバグ修正・命名整理・コメント追加は作業者判断で進めてよい
- 1フェーズ = 1コミット。ビルドが通ることを確認してからコミットする
- フェーズをまたいで複数の責務を同時に実装しない
- 各フェーズ完了後に「次の候補」を提示して私の承認を得てから次へ進む
- **既存の動作を壊す変更はしない。常に追加方式で進める**

---

## 1. ソフトの目的

**個人制作向け本格手描き2Dアニメ制作ソフト（OpenToonz的なもの）**

- 最終表現は手描き2Dアニメ。CGアニメを作るソフトではない
- 3DCGは「作画補助」として使う（下敷き・定規・カメラ計算・動きのガイド）
- 3Dは完成映像には出力しない
- 対応OS: Windows（将来的にmac/Linuxも視野に入れた設計）
- 動作目標: **常時60fps**
- UIの言語: 日本語（将来的に英語切り替え対応）

---

## 2. 技術スタック（固定）

```
言語:          C++20
ビルド:        CMake + Ninja
ウィンドウ:    SDL3 (zlib)
UI:            Dear ImGui (MIT)
グラフィクス:  SDL3 Renderer（Phase1〜2）
               → OpenGL（Phase2.5 3D導入時に追加）
```

### ライブラリ一覧

| ライブラリ | 用途 | ライセンス | 導入フェーズ |
|-----------|------|-----------|------------|
| SDL3 | ウィンドウ・入力・テクスチャ | zlib | 導入済み |
| Dear ImGui | UIパネル全般 | MIT | 導入済み |
| stb_image_write | PNG書き出し | MIT/Public Domain | 導入済み |
| nlohmann/json | プロジェクトJSON保存 | MIT | 導入済み |
| libmypaint | ブラシエンジン（入り抜き・筆圧・水彩） | ISC | Phase 1.5（進行中） |
| GLM | ベクトル・行列・カメラ計算 | MIT | Phase 2.5 |
| OpenGL + GLAD | 3Dシーン描画 | ロイヤリティフリー/MIT | Phase 2.5 |

### ライセンス方針（厳守）
- 使用可能: MIT / BSD-2 / BSD-3 / Apache-2.0 / zlib / ISC / Public Domain
- 要確認: LGPL系（必ず私に報告してから追加）
- 禁止: GPL専用ライブラリ・出所不明コード
- FFmpeg: 外部ツールとしてコマンド呼び出しのみ

**新しいライブラリを追加するときは必ず以下を説明してから追加する:**
ライブラリ名 / 用途 / ライセンス / 代替手段 / 無償公開時の注意点 / 公式URL

---

## 3. 設計原則

### 3-1. レイヤード・アーキテクチャ

```
┌──────────────────────────────────────┐
│  UI層 (Dear ImGui パネル群)           │ ← データを表示・編集するだけ
├──────────────────────────────────────┤
│  アプリ層 (App)                       │ ← 状態管理・入力の受け取り
├──────────────────────────────────────┤
│  ドメイン層 (Scene/Cut/Cell/Frame)    │ ← データ構造・ロジック（UIを知らない）
├──────────────────────────────────────┤
│  レンダリング層 (CanvasBitmap)        │ ← ピクセル操作・GPU転送
└──────────────────────────────────────┘
```

禁止: UI層がレンダリング層を直接呼ぶ / ドメイン層がImGuiを使う

### 3-2. 描画の60fps設計（変えない）

```
毎フレーム（軽い）:
  dirty=false なら SDL_Texture に触らない
  ImGui::Image() で既存テクスチャを貼るだけ
  描画中のストロークのみ DrawList::AddLine で描く

ストローク確定時のみ（重くてよい）:
  bakeStroke() でピクセルに焼く
  SDL_Texture の dirty 矩形だけアップロード

絶対禁止:
  毎フレーム全ストロークを DrawList に積む
  毎フレーム SDL_Texture を作り直す
  キャッシュ層を2層以上作る
  描画ループ中に重い計算を行う
```

### 3-3. ファイル分割ルール

**「1ファイル1責務」を守る。行数は目安であって制限ではない。**

分割を検討するタイミング:
- 異なる概念が同居している
- 1つのファイルを変更すると関係ない機能に影響が出る
- 800行を超えてまだ増え続けている

### 3-4. 追加方式の原則

既存の動作を壊さない。新機能は必ず「追加」で実装する。

### 3-5. 保存形式の方針

**過去のプロジェクトファイルとの互換性は考慮しない。**
開発中のため、保存形式を変更したときは既存プロジェクトが開けなくなってよい。

- 全JSONに `"format_version": 1` を追加する（将来の参考のため）
- フィールドの読み込みはデフォルト値付き `value("key", default)` で統一する
- 移行関数・旧形式検出・format_versionによる分岐処理は作らない

---

## 4. アニメ制作の現場ワークフローと本ソフトの対応

```
現場のフロー              本ソフトでの対応
─────────────────────────────────────────────────────────────────
① 企画・設定              Phase 4: プロジェクト管理・設定資料
② 絵コンテ               Phase 4: 絵コンテモード
③ レイアウト（ラフ原）   作画モード→動画サブ→[レイアウト]（Phase 2前）
  └ タイムシート記入      作画モード→タイムシートUI（Phase 2前）
  └ 背景美術も並行開始    作画モード→背景美術サブ→[レイアウト]
④ 原画                   作画モード→動画サブ→[原画]
⑤ 動画（中割り）         作画モード→動画サブ→[動画]
⑥ クリンナップ           Phase 1: ペン・消しゴム（完了）
⑦ 色トレス・影指定       Phase 1.5: 色トレスレイヤー
⑧ 彩色（仕上げ）         彩色モード（Phase 1.5後半）
⑨ 撮影・合成             Phase 3: 撮影モード
⑩ 書き出し               Phase 1: PNG/MP4（完了）
```

---

## 5. データモデル（C++側の階層構造）

```
Project
└── scenes: vector<Scene>
    ├── id, name, memo
    └── cuts: vector<Cut>
        ├── id, name, durationFrames, fps, memo
        ├── camera（2Dカメラ設定）
        ├── timesheet（タイムシート情報）← セクション10で詳述
        ├── cellZOrderKeys（フレームごとのzOrder変更）← セクション10で詳述
        └── cells: vector<Cell>
            ├── id, name, category
            ├── widthPx, heightPx
            ├── zOrder           ← デフォルト値。cellZOrderKeys で上書き可能
            ├── placement（位置・回転・拡大）
            ├── motionKeys（キーフレーム）
            └── frames: vector<Frame>
                ├── name
                └── layers: vector<Layer>
                    ├── name, visible, opacity
                    ├── type: LayerType
                    └── strokes: vector<Stroke>
                        ├── color: array<float,4>  ← RGBA 0.0〜1.0
                        ├── radiusPx: float
                        ├── brushEngine: StrokeBrushEngine
                        ├── hardness: float
                        ├── spacing: float
                        ├── pressureToSize: float
                        ├── pressureToOpacity: float
                        ├── watercolorBleed: float
                        ├── colorMix: float
                        ├── dryRate: float
                        └── points: vector<StrokePoint>
                            └── x, y, pressure
```

### LayerType

```cpp
enum class LayerType {
    Normal,       // 通常線画レイヤー（黒い実線）← Phase 1 実装済み
    Rough,        // ラフレイヤー（下書き・デフォルト50%透明）← Phase 1.5
    ColorTrace,   // 色トレスレイヤー（塗り分け境界線）← Phase 1.5
    Paint,        // 彩色レイヤー（バケツ塗り専用）← Phase 1.5
    ShadowGuide,  // 影指定レイヤー（最終出力に含めない参照用）← Phase 1.5
    Layout,       // レイアウトレイヤー（構図・背景の設計図）← 作画モード実装時
};
```

---

## 6. プロジェクトのフォルダ構成

```
my_anime_project/
│
├── project.json                 ← 作品全体の設定
├── app_state.json               ← UI状態（非正規データ・なくても開ける）
│
├── scenes/
│   ├── scene_001/
│   │   ├── scene.json           ← シーン名・メモ・cutOrder
│   │   └── cuts/
│   │       ├── cut_001/
│   │       │   ├── cut.json     ← 尺・fps・カメラ・タイムシート・cellOrder
│   │       │   └── cells/
│   │       │       ├── cell_A/
│   │       │       │   ├── cell.json
│   │       │       │   └── frames/
│   │       │       │       ├── frame_001/   ← 原画①（表示開始フレーム番号がフォルダ名）
│   │       │       │       │   ├── frame.json
│   │       │       │       │   ├── layer_001.json  ← 線画（Normal）
│   │       │       │       │   ├── layer_002.json  ← 色トレス（ColorTrace）
│   │       │       │       │   └── layer_003.json  ← 彩色（Paint）
│   │       │       │       ├── frame_003/   ← 中割り（フレーム3に表示）
│   │       │       │       │   └── layer_001.json
│   │       │       │       └── frame_007/   ← 原画②（フレーム7に表示）
│   │       │       │           └── ...
│   │       │       └── cell_BG/
│   │       │           ├── cell.json
│   │       │           └── frames/
│   │       │               └── frame_001/
│   │       │                   └── layer_001.json
│   │       └── cut_002/
│   │           └── ...
│   └── scene_002/
│       └── ...
│
├── assets/
│   ├── palettes/
│   │   └── palette.json
│   ├── storyboard/              ← Phase 4
│   │   ├── storyboard.json
│   │   └── panels/
│   └── settings/               ← Phase 4（設定資料）
│       ├── characters/
│       └── backgrounds/
│
└── exports/                     ← git管理外
    └── scene_001_cut_001/
        ├── png_sequence/
        └── line_test/
```

**フレームフォルダ名の規則:**
フォルダ名 `frame_NNN` の `NNN` は**タイムライン上の表示開始フレーム番号（1始まり）**。
`cut.json` の `timesheet` を参照すれば各フォルダが何コマ目に表示されるか一意に決まる。

---

## 7. 各JSONの構造

### project.json

```json
{
  "format_version": 1,
  "name": "作品タイトル",
  "canvas": { "width": 2560, "height": 1440 },
  "output": { "width": 1920, "height": 1080, "fps": 24, "pixelAspect": 1.0 },
  "audio": { "enabled": false, "filePath": "", "startFrame": 0 },
  "sceneOrder": ["scene_001", "scene_002"]
}
```

### scene.json

```json
{
  "format_version": 1,
  "id": "scene_001",
  "name": "冒頭シーン",
  "memo": "",
  "cutOrder": ["cut_001", "cut_002"]
}
```

### cut.json

```json
{
  "format_version": 1,
  "id": "cut_001",
  "name": "C001",
  "durationFrames": 12,
  "fps": 24,
  "memo": "",

  "camera": {
    "centerX": 1280.0, "centerY": 720.0, "zoom": 1.0,
    "animationEnabled": false, "keys": []
  },

  "cellOrder": ["cell_A", "cell_BG"],

  "timesheet": {
    "defaultExposure": 1,
    "cells": {
      "cell_A": {
        "exposure": 1,
        "entries": [
          { "frame": 1, "type": "key",       "number": 1 },
          { "frame": 3, "type": "inbetween"              },
          { "frame": 5, "type": "inbetween"              },
          { "frame": 7, "type": "key",       "number": 7 },
          { "frame": 9, "type": "inbetween"              }
        ]
      },
      "cell_BG": {
        "exposure": 1,
        "entries": [
          { "frame": 1, "type": "key", "number": 1 }
        ]
      }
    },
    "cameraInstructions": [
      { "frame": 5, "instruction": "PAN →", "value": null }
    ]
  },

  "cellZOrderKeys": [
    { "frame": 1,  "order": ["cell_BG", "cell_A"] },
    { "frame": 7,  "order": ["cell_A", "cell_BG"] }
  ]
}
```

**timesheetフィールドの意味:**

| フィールド | 意味 |
|-----------|------|
| `defaultExposure` | カット全体の基本コマ打ち（1=フル、2=2コマ、3=3コマ） |
| `cells[id].exposure` | セルごとの基本コマ打ち（defaultExposureを上書き） |
| `entries[].frame` | タイムライン上のフレーム番号（1始まり） |
| `entries[].type` | `"key"`=原画 / `"inbetween"`=中割り / `"null"`=空セル |
| `entries[].number` | `"key"`のときのみ。フォルダ名 `frame_NNN` の番号と一致する |
| `cameraInstructions` | カメラ欄の指示（撮影モードのキーフレームと連動） |
| `cellZOrderKeys` | フレームごとのセル重なり順（配列の先頭が最下層） |

**`entries`とフォルダの対応（上のサンプルの場合）:**
```
cell_A のエントリ:
  frame=1, type="key", number=1   → frames/frame_001/ を表示（原画①）
  frame=3, type="inbetween"       → frames/frame_003/ を動画マンが描く
  frame=5, type="inbetween"       → frames/frame_005/ を動画マンが描く
  frame=7, type="key", number=7   → frames/frame_007/ を表示（原画②）
  frame=9, type="inbetween"       → frames/frame_009/ を動画マンが描く
```

### cell.json

```json
{
  "format_version": 1,
  "id": "cell_A",
  "name": "キャラA",
  "category": "character",
  "widthPx": 1920,
  "heightPx": 1080,
  "zOrder": 1,
  "visible": true,
  "opacity": 1.0,
  "placement": { "x": 0.0, "y": 0.0, "scale": 1.0, "rotation": 0.0 },
  "motionKeys": []
}
```

### layer_NNN.json（ストローク点列が正データ）

```json
{
  "format_version": 1,
  "layerId": "layer_001",
  "name": "線画",
  "type": "Normal",
  "visible": true,
  "opacity": 1.0,
  "blendMode": "normal",
  "strokes": [
    {
      "color": [0.05, 0.05, 0.05, 1.0],
      "radiusPx": 4.0,
      "brushEngine": "MyPaint",
      "hardness": 1.0,
      "spacing": 0.25,
      "pressureToSize": 0.5,
      "pressureToOpacity": 0.0,
      "watercolorBleed": 0.0,
      "colorMix": 0.0,
      "dryRate": 1.0,
      "points": [
        {"x": 100.0, "y": 200.0, "pressure": 0.8},
        {"x": 105.0, "y": 205.0, "pressure": 0.9}
      ]
    }
  ]
}
```

`composite.png` は作らない。点列から毎回再ベイクする。

---

## 8. ソースコード構造

```
src/
├── app/
│   └── main.cpp
├── core/
│   ├── StrokePoint.h
│   ├── Stroke.h / .cpp
│   ├── Layer.h / .cpp
│   ├── Frame.h / .cpp
│   ├── Cell.h / .cpp
│   ├── Cut.h / .cpp            ← Timesheet・CellZOrderKey 構造体を含む
│   ├── Scene.h / .cpp
│   ├── Project.h / .cpp
│   ├── Brush.h / .cpp
│   └── WorkCanvas.h
├── render/
│   ├── CanvasBitmap.h / .cpp
│   └── CanvasRenderer.h / .cpp
├── brush/
│   ├── BrushEngine.h
│   ├── SimpleBrushEngine.h / .cpp
│   └── MyPaintBrushEngine.h / .cpp
├── fill/
│   └── FloodFill.h / .cpp
├── camera/
│   ├── ShotCamera2D.h / .cpp
│   └── CameraAnimation.h / .cpp
├── scene3d/                    ← Phase 2.5 で追加
│   ├── Scene3D.h / .cpp
│   ├── BoneCharacter.h / .cpp
│   ├── Primitive3D.h / .cpp
│   └── PreviewCamera3D.h / .cpp
├── physics/                    ← Phase 5 で追加
│   ├── HairSimulation.h / .cpp
│   └── ClothSimulation.h / .cpp
├── io/
│   ├── ProjectIO.h / .cpp
│   ├── PngExporter.h / .cpp
│   └── FfmpegRunner.h / .cpp
└── ui/
    ├── App.h / .cpp
    ├── AppDrawingMode.cpp
    ├── AppInput.cpp
    ├── AppOperations.cpp
    ├── AppProjectIO.cpp
    ├── CanvasView.h / .cpp
    └── panels/
        ├── LayerPanel.h / .cpp
        ├── FramePanel.h / .cpp
        ├── TimelinePanel.h / .cpp
        ├── TimesheetPanel.h / .cpp  ← 作画モード実装時に追加
        ├── BrushPanel.h / .cpp
        ├── ColorPanel.h / .cpp
        ├── OnionSkinPanel.h / .cpp
        ├── CellPanel.h / .cpp       ← Phase 2 で追加
        ├── Scene3DPanel.h / .cpp    ← Phase 2.5 で追加
        ├── StoryboardPanel.h / .cpp ← Phase 4 で追加
        └── ExportPanel.h / .cpp
```

---

## 9. UI構成

### モード切り替え（タブバー）

```
① プロジェクト | ② 絵コンテ | ②.5 プリビズ | ③ 作画 | ③.5 彩色 | ④ 撮影 | ⑤ 出力
```

対応する AppMode:

```cpp
enum class AppMode : int {
    Project = 0,
    Storyboard,
    Previs,
    Drawing,
    Coloring,   ← Drawing と Shooting の間
    Shooting,
    Export,
};
```

### カラーテーマ（ダーク固定）

```
背景（最暗部）: #0d0d10
パネル背景:     #141418
キャンバス背景: #0f0f12
ボーダー:       #2a2a2f (0.5px)
テキスト（主）: #cccccc
テキスト（補助）:#666666
アクセント:     #7F77DD（紫系）
```

### モード別デフォルトレイアウト

**① プロジェクトモード**
```
左: シーン・カット一覧（ツリー表示）
中: カットのサムネイル・情報（尺・コメント）
下: 全カットを横並びにした簡易タイムライン
```

**② 絵コンテモード**
```
左大: 絵コンテキャンバス（左絵/右説明の紙面レイアウト）
右上: コマ一覧
右下: 説明入力欄（台詞・アクション・カメラ・音・メモ）
```

**②.5 プリビズモード**
```
作画モードと同じ構成。
左サイドバーに「3Dオブジェクト」タブを追加。
キャンバス背面に3Dシーンを半透明で重ねて表示。
```

**③ 作画モード**
```
┌──────────┬──────────────────────────┬──────────────┐
│ 左サイド  │    キャンバス             │  右サイド    │
│ ツール    │    （中央・大）           │  セル一覧    │
│ ブラシ設定│                          │  レイヤー    │
│ カラー    │                          │  フレーム    │
│ オニオン  │                          │  書き出し    │
│ ライトTBL │                          │              │
├──────────┴──────────────────────────┴──────────────┤
│ タイムライン / タイムシート（タブで切り替え）         │
└───────────────────────────────────────────────────────┘
```

作画モード内のサブモード切り替えボタン（上部）:
```
[動画 ▼]  [レイアウト] [原画] [動画] [→彩色]
[背景美術 ▼]  [レイアウト] [背景作画] [BOOK作成]
```

**③.5 彩色モード**
```
┌──────────┬──────────────────────────┬──────────────┐
│ 左サイド  │    キャンバス（彩色ビュー）│  右サイド    │
│ カラー    │                          │  セル一覧    │
│ バケツ塗り│                          │  レイヤー    │
│ スポイト  │                          │  （種別表示）│
│ ブラシ    │                          │  フレーム    │
├──────────┴──────────────────────────┴──────────────┤
│ タイムライン                                         │
└───────────────────────────────────────────────────────┘
```

**④ 撮影モード**
```
作画モードと同じ構成。
右サイドバーに撮影台パネルを追加:
  Z値（奥行き）・被写界深度・グロー・色調補正・合成モード
```

**⑤ 出力モード**
```
上: フレームサムネイル一覧（スクロール可・範囲指定）
左下: 書き出し設定（形式・解像度・fps・出力先・FFmpegパス）
右下: 進捗・ログ
```

### パネル配置方式

- ImGui::DockSpace を使ってドラッグで自由に移動・リサイズ・ドッキング可能
- モードタブ右端に「レイアウトリセット▼」ボタン
- ウィンドウメニューから任意のパネルをどのモードでも呼び出せる

---

## 10. 作画モードの詳細仕様

### 10-1. サブモードの構造

```
[動画] サブモード
├── [レイアウト]   構図・セル定義・タイムシート記入・プリビズ配置
├── [原画]         キーとなる絵を描く・ライトテーブル参照可
├── [動画]         中割りを描く・中割ライトテーブル自動表示
└── [→彩色]        彩色モードへ移動するボタン

[背景美術] サブモード
├── [レイアウト]   キャラレイアウトを参照しながら背景設計・カメラ画角設定
├── [背景作画]     背景を描く・合成プレビュー参照可
└── [BOOK作成]     部分的な美術素材（ドア・小道具等）を作る
```

各サブモードで使えるツールとレイヤー表示ルールが変わる（データは変わらない）。

### 10-2. タイムシートUI

現場の紙タイムシートに準拠した縦型グリッドUI。
縦軸 = 時間（フレーム番号）、横軸 = セル列。

```
┌──────┬───┬───┬────┬────────────┐
│フレーム│ A  │ B  │ BG │  カメラ指示  │
│番号   │セル│セル│    │              │
├──────┼───┼───┼────┼────────────┤
│   1   │ ① │ ① │ ①  │              │
│   2   │    │    │    │              │
│   3   │  ・│    │    │              │  ← ・= 中割り位置
│   4   │    │    │    │              │
│   5   │  ・│    │    │  PAN →      │
│   6   │    │    │    │              │
├──────┼───┼───┼────┼────────────┤  ← 6コマごとに太線
│   7   │ ② │ ② │    │              │
│   8   │  ・│  ・│    │              │
│   9   │  ・│  ・│    │              │
…
```

| 記号 | 意味 |
|------|------|
| ① ② ③ … | 原画番号（丸付き数字）。この絵を描く |
| ・ | 中割り位置。動画への指示 |
| × | 空セル（このフレームは透明） |
| 空欄/棒線 | 直前の絵をそのままホールド |

**fps・コマ打ちの設定（パネル上部）:**
```
fps:         [24] [30]         ← カット単位で設定（cut.json に保存）
基本コマ打ち: [1コマ] [2コマ] [3コマ]  ← セルごとに個別設定可能
```

フルアニメーション目標のため1コマ打ちを主に使う。
同一カット内でも動きに応じて混在可能。

**カメラ欄の指示記号:**

| 記号 | 意味 |
|------|------|
| PAN → / ← | 左右パン |
| T.U. / T.B. | ズームイン / ズームアウト |
| FOLLOW | 被写体追従 |
| F.I. / F.O. | フェードイン / フェードアウト |

これらは撮影モードのカメラキーフレームと連動する。

**タイムシート → タイムラインへの自動反映:**
```
タイムシートで指定:
  A列: frame=1 に①, frame=3 に・, frame=5 に・, frame=7 に②

タイムラインに自動反映:
  フレーム1〜2: frames/frame_001/（原画①）を表示
  フレーム3〜4: frames/frame_003/（中割り）を表示
  フレーム5〜6: frames/frame_005/（中割り）を表示
  フレーム7〜 : frames/frame_007/（原画②）を表示

中割り位置（・）のフレームを動画サブモードで開くと:
  前の原画と次の原画が中割ライトテーブルとして自動表示される
```

**セルのzOrder変更:**
カット途中でセルの重なり順が変わる場合、タイムシートのカメラ欄または
専用列に記入する。そのフレームから `cellZOrderKeys` でzOrderが切り替わる。

### 10-3. オニオンスキンと中割ライトテーブルの切り替え

左サイドバーに切り替えボタンを設置。どちらか一方・または両方を同時に使える。

| 機能 | 挙動 | ショートカット |
|------|------|---------------|
| オニオンスキン | 前後Nフレームを青/赤で薄く表示（フレーム番号基準） | O |
| 中割ライトテーブル | タイムシートの前後の原画フレームを自動取得して表示（原画位置基準） | L |

中割ライトテーブルは Phase 2（タイムシート実装後）に実装する。

### 10-4. 合成プレビュー（背景美術モード）

```
キャンバス表示の構成（下から）:
  1. 背景レイヤー（編集対象）         ← フル表示・編集可
  2. キャラAのレイアウト（参照）      ← 30〜50%透明・編集不可
  3. キャラBのレイアウト（参照）      ← 同上

左サイドバーに:
  「キャラ参照」ON/OFF トグル
  参照不透明度スライダー（0〜100%）
  参照するセルの選択
```

### 10-5. プリビズとの連携（Phase 2.5以降）

各作業段階で3Dシーンを下敷きとして使える:
- 「プリビズ参照」ON/OFF で切り替え可能
- 背景美術のレイアウト段階で3Dカメラの画角を自動取得できる

---

## 11. 彩色モードの詳細仕様

彩色モードに切り替えてもデータは変わらない。表示とツールが変わるだけ。

| 項目 | 作画モード | 彩色モード |
|------|-----------|-----------|
| デフォルト編集対象 | Normal レイヤー | Paint レイヤー |
| Normal レイヤー | 通常編集可 | 30〜50%透明で表示（下絵参照） |
| ColorTrace レイヤー | 編集可 | 50〜70%透明で表示（境界参照） |
| Paint レイヤー | 編集可 | メインで編集（バケツ塗り） |
| ShadowGuide レイヤー | 編集可 | 参照専用（編集不可） |
| デフォルトツール | ブラシ | バケツ塗り |
| キャンバス背景 | 透明チェッカー | 白（塗り確認用） |

`AppMode::Coloring` に切り替えたとき、Paint レイヤー（なければ作成）に自動移動する。

---

## 12. フェーズ別実装計画

---

### ✅ Phase 0: リポジトリ初期化（完了）

SDL3 + ImGui のウィンドウ表示。モードタブ骨格。ダークテーマ定数。

---

### ✅ Phase 1: 作画コア（完了）

- CanvasBitmap（ピクセルバッファ＋SDL_Texture）
- CanvasRenderer（フレーム合成・ビューポート表示）
- ペン描画・消しゴム（ストローク部分消去）
- 複数レイヤー・複数フレーム・Undo/Redo
- オニオンスキン（前後フレームを色付きで表示）
- プロジェクト保存・読み込み（JSON）
- PNG連番・MP4書き出し（FFmpeg外部呼び出し）
- タイムライン（フレーム切り替え・スクロール）

---

### Phase 1.5: 作画品質向上（進行中）

#### 完了済み
- libmypaint 接続（逐次処理・Undo/保存後の描き味保持）
- ブラシパラメータ（筆圧・硬さ・間隔・水彩）
- バケツ塗り（FloodFill・隙間閉じ・漏れ防止）
- カラーパレット保存・読み込み
- ライトテーブル（任意フレームの半透明重ね表示）
- 再生機能（フレームを順番に再生）
- 書き出しモード（LineTest / ColorTrace / LineOnly）
- LayerType（Normal / ColorTrace / Paint / ShadowGuide / Rough）

#### 未完了（この順番で実装する）

**Step 16: format_version の追加**
- 全JSON（project/scene/cut/cell/frame/layer）に `"format_version": 1` を追加
- 移行処理は作らない。既存プロジェクトが開けなくなっても問題ない

**Step 17: 彩色モードの追加**
- `AppMode::Coloring` を Drawing と Shooting の間に追加
- タブバーに「③.5 彩色」タブを追加
- `drawDrawingMode()` を流用してレイヤー表示ルールとデフォルトツールを変える
- 別の巨大な描画関数を作らない

**Step 18: ColorTrace の最終出力処理**
- Composite 書き出し時に ColorTrace 線を隣接 Paint 色で置換
- `PngExporter::rasterizeFrame()` の Composite モード処理内に実装

#### 書き出しモード

```cpp
enum class ExportMode {
    Composite,  // 全レイヤー合成・白背景（実装済み）
    LineTest,   // Normal のみ・白背景（鉛筆アニメ）（実装済み）
    ColorTrace, // Normal + ColorTrace・白背景（色トレスアニメ）（実装済み）
    LineOnly,   // Normal のみ・背景透明（素材書き出し）（実装済み）
};
```

#### libmypaintについて
- ライセンス: ISC（MIT相当、商用利用完全無料）
- 採用実績: Krita・GIMP・OpenToonz
- 公式: https://github.com/mypaint/libmypaint
- 導入: vcpkg (`vcpkg install libmypaint:x64-windows`)

---

### Phase 2: ファイル構成改定 + セル概念の導入

**Phase 2 前に必ず実施: ファイル構成の改定**

現在の構成（`cells/` 直下）を `scenes/scene_NNN/cuts/cut_NNN/cells/` に変更する。
データの中身は変わらない。パスだけ変わる。

```
追加する C++ ファイル:
  src/core/Scene.h / .cpp
  src/core/Cut.h / .cpp（Timesheet・CellZOrderKey 構造体を含む）

Project.h の変更:
  vector<Cell> cells → vector<Scene> scenes

App.h への追加:
  string activeSceneId_
  string activeCutId_

ProjectIO.cpp のパス変更:
  旧: folderPath / "cells" / cell.id / ...
  新: folderPath / "scenes" / scene.id / "cuts" / cut.id / "cells" / cell.id / ...
```

**セル概念の実装:**

```
CellPanel（新規）:
  セル一覧（ドラッグで重ね順変更）
  新規セル追加（名前・カテゴリ・キャンバスサイズ）
  セルの表示/非表示・不透明度
  配置（位置X/Y・拡大縮小・回転）

カテゴリ: キャラ・背景・エフェクト・ブック（小道具）・タイトル

Cell.zOrder はデフォルト値。
cut.json の cellZOrderKeys でフレームごとに上書き可能。

セルモーションキーフレーム:
  位置X/Y・回転・拡大をフレームごとに保存
  補間方式: 線形 / スムーズ / ステップ
  cell.json の motionKeys に保存
```

**タイムシートとタイムラインの連動実装（Phase 2 内）:**
```
Step A: Cut.h に Timesheet 構造体追加、ProjectIO で読み書き
Step B: タイムシートUI実装（縦型グリッド・fps・コマ打ち設定）
Step C: タイムシート → タイムライン連動
Step D: cellZOrderKeys 実装（フレームごとのzOrder切り替え）
Step E: 中割ライトテーブル実装（前後原画の自動取得・表示）
```

**Phase 2 完了条件:**
- 複数セルを重ねて表示できる
- セルの位置・回転・拡大をキーフレームで動かせる
- タイムシートUIでコマ割りを指定できる
- タイムシートとタイムラインが連動する

---

### Phase 2.5: プリビズ（ソフト内蔵3Dシーンエディタ）

ソフト内部でOpenGL 3Dシーンを編集・再生し、作画の下敷きにする。
外部ソフトは不要。

**技術:**
```
SDL3のOpenGLコンテキスト（SDL_GL_CreateContext）
Dear ImGuiをOpenGLバックエンドに切り替え（imgui_impl_opengl3）
GLADでOpenGL関数ロード
3DビューをFBOにレンダリング → ImGui::Image()で表示
GLMで行列・ベクトル計算
```

**配置できるオブジェクト（数・サイズ・配置を自由に設定）:**
```
ボーン人形（humanoidリグ）
  頭・胴・腰・上腕・前腕・手・大腿・下腿・足
  各関節の回転をスライダーで設定
  ポーズをフレームごとにキーフレーム保存

プリミティブ（各軸を個別スケール可能）
  直方体（W・H・D を各軸で個別指定）
  球（半径）/ 円柱（半径・高さ）/ 平面

シーン補助
  床グリッド / パースガイド線 / 撮影フレーム表示
```

**3Dカメラ（実際の映画撮影に近い設定）:**
```cpp
struct PreviewCamera3D {
    float focalLengthMm    = 35.0f;
    float sensorWidthMm    = 36.0f;   // 35mmフルサイズ = 36mm
    float sensorHeightMm   = 24.0f;
    float anamorphicSqueeze = 1.0f;   // 1.0=球面 / 1.33=1.33x / 2.0=2x
    // FOV = 2 * atan(sensorWidth / (2 * focalLength))
    glm::vec3 position = {0.0f, 1.6f, 5.0f};
    glm::vec3 target   = {0.0f, 1.0f, 0.0f};
    float rollDeg = 0.0f;
};
```

ドーリーズーム: `focalLength / distance = constant`

**作画との連携:**
- 3Dシーンを半透明でキャンバス背面に表示（透過度調整可）
- フレームと同期して3Dが動く
- 背景美術のレイアウト段階で画角を自動取得
- 保存: `scene3d/scene.json`

---

### Phase 3: 撮影台（マルチプレーン・エフェクト）

**マルチプレーン:**
```
セルに Z値（奥行き）を付与
カメラとZ値の差で拡大率・パン追従が変わる（遠近感・視差）
```

**被写界深度:**
```
計算式: CoC = |f² / (N*(D-f))| * |D - Ds| / Ds
  f=焦点距離(mm), N=F値, D=被写体距離, Ds=ピント面距離

簡易版: CoC値に応じたガウシアンブラー
本格版: 絞り羽根形状・アナモルフィック楕円ボケ（将来）
```

**透過光・撮影エフェクト（GLSLシェーダー）:**
```
グロー / ハレーション / ゴッドレイ（簡易版から実装）
合成モード: 通常 / 加算 / 乗算 / スクリーン
色調補正: 明るさ・コントラスト・彩度・ビネット
将来: レンズフレア・コマ収差・色収差
```

---

### Phase 4: プリプロダクション

**絵コンテ（作品単位・ジブリ様式参考）:**
```
左: 絵を描く四角（目安・はみ出してもよい）
右: 台詞・アクション・カメラ・音・メモの入力欄
最低4コマ・任意コマ数
PNG書き出し・将来は印刷対応
保存: assets/storyboard/storyboard.json
```

**設定資料・カラーパレット:**
```
イメージボード・キャラクター設定・背景設定
カラーパレット（設定資料・キャンバスからスポイト生成）
保存: assets/palettes/palette.json
```

---

### Phase 5: 簡易物理（作画補助ガイド）

Verlet積分による物理シミュレーション。最終映像には出力しない。

```cpp
// position_new = 2*position - position_old + acceleration * dt²
struct Particle {
    glm::vec2 position;
    glm::vec2 positionPrev;
    float mass = 1.0f;
    bool pinned = false;
};
```

実装するガイド: 髪・服の裾。風・重力・空気抵抗。ベイク後に手修正可能。

---

## 13. 出力形式

```
Phase 1（実装済み）:
  PNG / PNG連番 / MP4（FFmpeg外部呼び出し）

書き出しモード（Phase 1.5 実装済み）:
  Composite（全レイヤー合成・白背景）
  LineTest（Normal のみ・白背景）← 鉛筆アニメ・ラインテスト
  ColorTrace（Normal + ColorTrace・白背景）← 色トレスアニメ・メイキング
  LineOnly（Normal のみ・背景透明）← 他ソフトへの素材

Phase 3 で追加:
  撮影合成結果のPNG連番 / WAV（音声書き出し）

将来:
  OpenEXR連番 / OTIO
```

---

## 14. ショートカット一覧

```
描画
  B: ブラシ   E: 消しゴム   G: バケツ塗り   [ / ]: ブラシサイズ縮小/拡大

表示
  H: 手（パン）   Z: ズーム   F: 全体表示   Tab: UIを隠してキャンバス全画面
  O: オニオンスキンON/OFF
  L: 中割ライトテーブルON/OFF（Phase 2 実装後）

フレーム操作
  Shift+← / Shift+→: 前後フレームに移動
  Space: 再生・停止
  Home / End: 最初・最後のフレーム

編集
  Ctrl+Z: Undo   Ctrl+Y: Redo   Ctrl+S: 保存
```

---

## 15. コードコメント方針

```cpp
// 悪い例
float f = 2 * atan(s / (2 * l));

// 良い例
// 焦点距離とセンサーサイズから画角（視野角）を求める。
// FOV = 2 * atan(sensorWidth / (2 * focalLength))
// 例: 焦点距離35mm・センサー幅36mm → FOV ≈ 54.4度
float fieldOfViewRad = 2.0f * std::atan(sensorWidthMm / (2.0f * focalLengthMm));
```

- ファイル冒頭にそのファイルの役割を書く
- 難しい座標変換・数式には説明コメント必須
- 変数名は略しすぎない（`f` より `focalLengthMm`）
- 魔法の数字に定数名をつける

---

## 16. 作業報告フォーマット（厳守）

### 作業後に出すもの

```
変更ツリー（追加/変更/削除）
成果物（ZIP + 個別ファイル）
ビルドコマンド（PowerShell）
自動起動コマンド
動作確認項目
pushコマンド
次の候補（おすすめ理由付き）
```

### ビルド＋自動起動（標準形式）

短いビルド（CMakeLists.txtを変えていない場合）:
```powershell
cmake --build .\build --config Debug
if ($LASTEXITCODE -eq 0) {
    $exe = Get-ChildItem .\build -Recurse -Filter perapera_anime_maker.exe | Select-Object -First 1
    if ($exe) { & $exe.FullName } else { Write-Host "exe not found" }
} else { Write-Host "Build failed" }
```

フルビルド（CMakeLists.txtを変えた場合）:
```powershell
if (Test-Path .\build) { Remove-Item .\build -Recurse -Force }
cmake -S . -B .\build
cmake --build .\build --config Debug
if ($LASTEXITCODE -eq 0) {
    $exe = Get-ChildItem .\build -Recurse -Filter perapera_anime_maker.exe | Select-Object -First 1
    if ($exe) { & $exe.FullName } else { Write-Host "exe not found" }
} else { Write-Host "Build failed" }
```

---

## 17. 管理ファイル

### WORK_LOG.md（作業のたびに更新）
```
## YYYY-MM-DD Phase X-Step Y: タイトル
### 作業概要 / 変更ファイル / 実装内容 / 未完了 / 次にやること / 判断待ち
```

### DECISIONS.md（大きな設計判断を記録）
```
## Decision XXX: タイトル
日付: / 理由: / 影響:
```

---

## 18. リポジトリ情報

```
リポジトリ: https://github.com/tatata93/perapera-anime-maker
ローカル:   C:\Users\tak01\github\perapera-anime-maker
exe:        C:\Users\tak01\github\perapera-anime-maker\build\bin\perapera_anime_maker.exe
現在地:     Phase 1.5 進行中（Step 16〜18 残り）
次:         Step 16（format_version追加）→ Step 17（彩色モード）→
            Step 18（ColorTrace出力）→ Phase 2（ファイル構成改定+セル）
```
---

## Appendix: Phase 2 completion roadmap and AI instructions

This section makes Phase 2 finishable as numbered implementation work. It is part of the spec and should be updated when later agents change scope.

### Phase 2 current target

Phase 2 finishes when the normal project path is `Project -> Scene -> Cut -> Cell`, and the active Cut owns the Timesheet, Cells, frame/layer data, and Cut camera metadata needed by save, load, preview, and export.

Phase 2 must not reintroduce Scene Plate / separate background panel behavior. Layout, background, BOOK, effect, and reference material remain normal Cells/categories controlled through the Timesheet path until a later explicit phase changes that decision.

### Completed Phase 2 structure checkpoints

- Phase 2-pre: Scene Plate path was removed from the plan; background/layout/BOOK/effect/reference are normal Cells.
- Phase 2 Step 1: minimal Project -> Scene -> Cut -> Cell model and active-Cut bridge were introduced.
- Phase 2 Step 2: new scene/cut/cell layout save/load path replaced direct AppProjectIO use of legacy ProjectIO save/load, and major large files were split back toward the 800-line guideline.
- Phase 2 Step 3-a: Cut-owned camera metadata bridge was added.
- Phase 2 Step 3-b: ProjectLayout save/load round-trip now covers Cut camera metadata.

### Required remaining Phase 2 steps

- Phase 2 Step 3-c: clarify this roadmap and provide a clear spec/instruction entry document. This is a documentation/control step and should not change runtime behavior.
- Phase 2 Step 3-d: audit and test Cut-owned Timesheet propagation through app save/load, preview selection, and export setup. Fix only concrete gaps; avoid startup scans or preview-file warmup.
- Phase 2 Step 3-e: connect frame-level cell order / `cellZOrderKeys` to resolved display and export order, with a focused test before UI expansion.
- Phase 2 Step 3-f: connect Cell motion keys to Cut/Timesheet resolved preview/export data. Keep interpolation and cache invalidation lightweight.
- Phase 2 Step 3-g: improve the vertical Timesheet UI for production use after the data path is protected. UI work must keep startup and project loading light.
- Phase 2 Step 3-h: Phase 2 closeout audit. Confirm no active app path depends on old ProjectIO save/load, no DrawingNewLayoutIO revival exists, key files follow the 800-line guideline or have a split plan, and Debug app build plus relevant selftests pass.

### Ongoing Phase 2 rules for all agents

- Keep compatibility with older in-development save formats only when it is cheap; new layout correctness has priority.
- Do not generate or scan preview files during startup or project load. Heavy preview artifacts may be created only after explicit user action or during export/playback work that needs them.
- Prefer small selftests for data resolver, save/load, and ordering behavior before adding more UI.
- Add a short policy doc for each numbered step under `docs/`.
- Update `WORK_LOG.md`, `DECISIONS.md`, `CHATGPT_HANDOFF.md`, and `docs/github_progress_for_chatgpt.md` before pushing.
- If a source file grows beyond roughly 800 lines, split model/helper logic before adding more feature bulk unless the step is an emergency build fix.

### User decisions that may be needed later

- Whether Cell motion key interpolation should initially be hold-only, linear, or selectable per key.
- Whether Timesheet editing should prioritize keyboard entry first or drag-and-drop first for the first production UI pass.
- Whether camera columns should live inside the Timesheet grid or in a side panel synchronized to the selected frame.
