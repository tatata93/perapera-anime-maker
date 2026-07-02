# perapera-anime-maker タイムシート仕様 v1

作成日: 2026-06-29  
対象プロジェクト: `perapera-anime-maker`  
対象機能: 正式タイムシート機能  
状態: 実装前仕様案

---

## 1. 目的

この仕様書は、`perapera-anime-maker` に正式なタイムシート機能を実装する前に、機能範囲、操作方法、UI、データ構造、表示解決ルール、保存形式、実装順を整理するためのものである。

前回の仮タイムシート実装では、CellPanel、保存処理、描画表示、PNG/MP4出力、フレーム操作が一度に結合され、作画フレームとタイムラインフレームの責務が混ざった。その結果、フレーム追加、彩色モード移行、描画表示の安定性を損ねた。

正式実装では、以下を基本方針とする。

- タイムシートは作画機能の中核として扱う。
- CellPanel にタイムシートUIを混ぜない。
- 作画フレームとタイムラインフレームを明確に分離する。
- 仕様、core、io、UI、表示反映、出力反映を段階的に実装する。
- 最初から撮影・カメラ・出力まで全部つなげず、安定した小さい単位で進める。

---

## 2. 用語定義

### 2.1 作画F

作画Fは、実際にユーザーが描いた絵の単位である。

例:

```text
Aセル 作画F1
Aセル 作画F2
Aセル 作画F3
Bセル 作画F1
```

作画Fは素材であり、映像上の何コマ目に出るかはタイムシートが決める。

### 2.2 タイムラインT

タイムラインTは、映像上の時間位置である。

例:

```text
T1
T2
T3
T4
T5
```

タイムラインTは、再生、プレビュー、PNG連番、MP4出力の基準になる。

### 2.3 タイムシート

タイムシートは、各タイムラインTにおいて、各セルがどの作画Fを表示するかを決める表である。

例:

```text
T    Aセル    Bセル    BG    撮影指示
1    1        ×        BG1
2    ｜       ×        BG1
3    2        1        BG1
4    ｜       ｜       BG1
5    3        2        BG1
```

### 2.4 セル

セルは、作画や素材の列である。

例:

```text
A
B
C
口
目
影
ハイライト
BG
BOOK
FX
```

### 2.5 セル名とカテゴリ

セル名とカテゴリは分ける。

```text
セル名:
  A
  B
  口
  目
  影

カテゴリ:
  キャラセル
  背景
  BOOK
  エフェクト
  撮影素材
```

セル名はユーザーが自由に決める。カテゴリは制作上の意味や表示補助に使う。

---

## 3. 基本方針

### 3.1 作画FとタイムラインTを混ぜない

禁止する考え方:

```text
タイムラインTを選ぶ
↓
activeFrameIndex を直接変更する
↓
作画FとTが同じものとして扱われる
```

正式実装では、以下のように分ける。

```text
activeDrawingFrameIndex:
  現在編集対象の作画F

activeTimelineFrameIndex:
  現在表示・再生確認しているタイムラインT
```

### 3.2 タイムシートはCut単位

タイムシートは Project 直下に直接置かない。正式には Cut に属する。

理想構造:

```text
Project
  Scene
    Cut
      cells
      timesheet
      cameraInstructions
      shootingInstructions
      cellZOrderKeys
```

### 3.3 CellPanelにタイムシートを混ぜない

CellPanel はセル管理だけを担当する。

CellPanelの責務:

```text
- セル追加
- セル削除
- セル名編集
- カテゴリ編集
- 表示/非表示
- 不透明度
- 重ね順
- 単独表示
```

TimesheetPanelの責務:

```text
- タイムラインT表示
- セル列表示
- 作画F番号入力
- ホールド指定
- 空セル指定
- 原画/中割指定
- セリフ欄
- カメラ指示欄
- 撮影指示欄
- メモ欄
```

---

## 4. タイムシートでできること

### 4.1 MVPで必須の機能

最初の正式実装で必要な機能は以下とする。

```text
- 縦型タイムシートUIを表示できる
- 横方向にセル列を表示できる
- 縦方向にタイムラインTを表示できる
- 各マスに作画F番号を入力できる
- ホールドを指定できる
- 空セルを指定できる
- 原画/中割を区別できる
- Tを選択できる
- T選択に応じてキャンバス表示を切り替えられる
- 再生時にタイムシート結果を使える
- PNG連番/MP4出力時にタイムシート結果を使える
```

### 4.2 MVPではやらないこと

最初の実装では以下をやらない。

```text
- 複雑なカメラキーフレーム編集
- 撮影効果の実処理
- 背景美術管理全体との完全接続
- BOOK素材管理全体との完全接続
- 絵コンテ/レイアウトとの完全接続
- 高度な範囲編集
- 音声波形同期
- 口パク自動割り当て
```

ただし、将来接続できるように、セリフ、カメラ、撮影指示、素材メモ欄は最初からデータ構造に入れられる設計にする。

---

## 5. UI仕様

### 5.1 基本レイアウト

タイムシートUIは縦型とする。

```text
縦軸:
  タイムラインT

横軸:
  セル列

追加欄:
  セリフ
  カメラ指示
  撮影指示
  背景・BOOK・素材メモ
  レイアウト接続
```

例:

```text
T    Aセル    Bセル    口    目    BG    BOOK    セリフ       カメラ      撮影
1    1        ×        1     1     BG1           ……
2    ｜       ×        ｜    ｜    BG1
3    2        1        2     ｜    BG1           「おい」     TU開始
4    ｜       ｜       ｜    2     BG1
5    3        2        3     ｜    BG1   BOOK1                 透過光
```

### 5.2 マスの表示

マスの表示記号は以下とする。

```text
数字:
  指定した作画Fを表示

｜:
  ホールド。直前の表示状態を維持

×:
  空セル。そのセルを非表示

K:
  原画属性

I:
  中割属性
```

画面上では、数字と属性を組み合わせてもよい。

例:

```text
1K
2I
3K
```

ただし、UIが見づらくなる場合は、属性を色・枠・小アイコンで表示する。

### 5.3 T選択

タイムシート上の行をクリックすると、そのTを選択する。

```text
選択中T:
  activeTimelineFrameIndex
```

Tを選択しても、作画Fを直接変更してはならない。

### 5.4 作画対象の明示

キャンバス上には、現在の表示Tと編集対象を明示する。

例:

```text
T: 12
表示: Aセル 作画F5
編集対象: Aセル 作画F5
```

複数セル表示時:

```text
T: 12
表示:
  Aセル 作画F5
  Bセル 作画F2
  BG BG1
編集対象:
  Aセル 作画F5
```

### 5.5 描画時の基本方針

最初の実装では、安全のため以下の仕様を推奨する。

```text
タイムシートT:
  表示・再生・出力の基準

作画F:
  実際に描く対象
```

T上で見えている絵に直接描けるようにしてもよいが、その場合は必ず「どの作画Fを編集しているか」を表示する。

---

## 6. 入力操作仕様

### 6.1 基本操作

```text
クリック:
  マスを選択

数字入力:
  作画F番号を入力

Enter:
  下のTへ移動

Tab:
  右のセル列へ移動

Shift + Tab:
  左のセル列へ移動

Delete / Backspace:
  入力を消す

X:
  空セル

H:
  ホールド

K:
  原画

I:
  中割
```

### 6.2 範囲操作

将来実装する操作:

```text
- ドラッグで範囲選択
- コピー
- ペースト
- 選択範囲をホールド化
- 選択範囲を空セル化
- 選択範囲を原画化
- 選択範囲を中割化
- 選択範囲を削除
```

### 6.3 連番配置

作画Fを連番で配置できるようにする。

例:

```text
開始作画F: 1
終了作画F: 4
打ち: 2コマ

結果:
T1  1
T2  ｜
T3  2
T4  ｜
T5  3
T6  ｜
T7  4
T8  ｜
```

### 6.4 2コマ打ち・3コマ打ち

タイムシート入力支援として必須候補。

```text
1コマ打ち:
  1, 2, 3, 4...

2コマ打ち:
  1, ｜, 2, ｜, 3, ｜...

3コマ打ち:
  1, ｜, ｜, 2, ｜, ｜, 3...
```

---

## 7. 表示解決ルール

### 7.1 Resolverの役割

タイムシートは直接描画しない。

core側の `TimesheetResolver` が、指定Tにおいて表示すべきセルと作画Fを解決する。

入力:

```text
- Cut
- Timesheet
- timelineFrame
- cellId
```

出力:

```text
- 表示するか
- どのセルか
- どの作画Fか
- 原画/中割/通常か
- 不透明度
- 重ね順
```

### 7.2 基本解決

指定T以前の最後の有効エントリを見る。

```text
数字:
  指定作画Fを表示

ホールド:
  直前の表示状態を維持

空セル:
  非表示

未入力:
  直前の表示状態を維持
```

例:

```text
T1  1
T2  未入力
T3  2
T4  ｜
T5  ×
T6  未入力
```

解決結果:

```text
T1: 作画F1 表示
T2: 作画F1 表示
T3: 作画F2 表示
T4: 作画F2 表示
T5: 非表示
T6: 非表示
```

### 7.3 空セル後の未入力

空セル後の未入力は、空セル状態を維持する。

```text
T5: ×
T6: 未入力
T7: 未入力
```

結果:

```text
T5: 非表示
T6: 非表示
T7: 非表示
```

### 7.4 不正な作画F番号

存在しない作画F番号を参照した場合は、落とさない。

処理方針:

```text
- 表示しない
- UI上で警告表示する
- データは勝手に削除しない
```

例:

```text
Aセルに作画F3までしかない
タイムシートでは作画F5を指定
```

表示:

```text
Aセル F5 不明/欠落
```

### 7.5 セル削除時

セルが削除されても、タイムシートデータを即時完全削除するかは慎重に扱う。

MVPでは以下を推奨する。

```text
- セル削除時に確認する
- タイムシート列も削除するか選ばせる
- 復元可能な設計にする
```

---

## 8. データ構造案

### 8.1 基本方針

保存データは密な全フレーム配列ではなく、記入されたマスを中心に持つ。

理由:

```text
- 長尺カットでも軽い
- 未入力とホールドを区別しやすい
- 紙タイムシートに近い
- 差分保存しやすい
```

### 8.2 C++モデル案

```cpp
namespace perapera {

enum class TimesheetEntryType {
    Drawing,    // 作画F指定
    Hold,       // ホールド明示
    Null,       // 空セル
};

enum class TimesheetDrawingRole {
    Normal,
    Key,
    Inbetween,
};

struct TimesheetEntry {
    int timelineFrame = 0;                  // 0始まりT
    std::string cellId;                     // 対象セル
    TimesheetEntryType type = TimesheetEntryType::Hold;
    TimesheetDrawingRole role = TimesheetDrawingRole::Normal;
    int drawingFrameIndex = -1;             // 0始まり作画F。Drawing以外は-1
    std::string memo;
};

struct TimesheetTrack {
    std::string cellId;
    std::vector<TimesheetEntry> entries;
};

struct TimesheetNote {
    int timelineFrame = 0;
    std::string dialogue;
    std::string cameraInstruction;
    std::string shootingInstruction;
    std::string materialMemo;
    std::string layoutLink;
};

struct Timesheet {
    int totalFrames = 144;
    int defaultExposure = 2;
    std::vector<TimesheetTrack> tracks;
    std::vector<TimesheetNote> notes;
};

struct ResolvedTimesheetCell {
    std::string cellId;
    bool visible = false;
    int drawingFrameIndex = -1;
    TimesheetDrawingRole role = TimesheetDrawingRole::Normal;
    float opacity = 1.0f;
    int zOrder = 0;
};

struct ResolvedTimesheetFrame {
    int timelineFrame = 0;
    std::vector<ResolvedTimesheetCell> cells;
    TimesheetNote note;
};

} // namespace perapera
```

### 8.3 0始まりと1始まり

内部データは0始まり、UI表示は1始まりとする。

```text
内部:
  timelineFrame = 0
  drawingFrameIndex = 0

UI:
  T1
  作画F1
```

---

## 9. 保存形式案

### 9.1 cut.json内に保存する

正式実装では `timesheet.json` 単体ではなく、Cutデータ内に含める。

例:

```json
{
  "formatVersion": 1,
  "cutId": "cut_001",
  "name": "カット001",
  "totalFrames": 144,
  "cells": [],
  "cellZOrderKeys": [],
  "timesheet": {
    "defaultExposure": 2,
    "tracks": [
      {
        "cellId": "cell_A",
        "entries": [
          {
            "t": 0,
            "type": "drawing",
            "drawingFrame": 0,
            "role": "key"
          },
          {
            "t": 1,
            "type": "hold"
          },
          {
            "t": 4,
            "type": "null"
          }
        ]
      }
    ],
    "notes": [
      {
        "t": 12,
        "dialogue": "やめろ！",
        "camera": "TU開始",
        "shooting": "透過光",
        "material": "BOOK前面"
      }
    ]
  }
}
```

### 9.2 互換性

過去の仮 `timesheet.json` は正式実装へ直接引き継がない。

必要なら後で変換ツールを作るが、MVPでは優先しない。

---

## 10. UIとcoreの責務分離

### 10.1 core

```text
- Timesheet
- TimesheetTrack
- TimesheetEntry
- TimesheetNote
- TimesheetResolver
- TimesheetEdit
- TimesheetNormalize
- Cut
```

### 10.2 io

```text
- cut.json 保存
- cut.json 読み込み
- timesheet項目のシリアライズ
- 互換読み込み
```

### 10.3 ui

```text
- TimesheetPanel
- マス表示
- 入力受付
- 選択範囲
- キーボード操作
```

### 10.4 App

```text
- activeDrawingFrameIndex
- activeTimelineFrameIndex
- 編集対象セル
- 表示対象T
- モード切替
- UI後のdeferredAction実行
```

---

## 11. 実装順

### Step 1: coreモデル

```text
- Cutモデルの最小追加
- Timesheetモデル追加
- TimesheetResolver追加
- UIなしで解決テスト
```

### Step 2: io

```text
- cut.json保存形式追加
- timesheet保存/読み込み
- 既存Project保存との衝突回避
```

### Step 3: TimesheetPanel表示

```text
- 縦型表を表示
- セル列表示
- T行表示
- 現在Tハイライト
- まだ編集は最小限
```

### Step 4: 最小編集

```text
- 数字入力
- ホールド
- 空セル
- 原画/中割
- T選択
```

### Step 5: キャンバス表示反映

```text
- activeTimelineFrameIndexを使って表示解決
- 解決結果をキャンバスに表示
- 編集対象作画Fを明示
```

### Step 6: 再生・出力反映

```text
- 再生時にTimesheetResolverを使う
- PNG連番に反映
- MP4出力に反映
```

### Step 7: 便利操作

```text
- 2コマ打ち
- 3コマ打ち
- 連番配置
- 範囲選択
- コピー/ペースト
```

---

## 12. 操作面の目標

正式タイムシートは、以下の操作感を目指す。

```text
- 紙タイムシートに近い
- キーボードだけでも入力できる
- マウスでも直感的に編集できる
- どのTを見ているか分かる
- どの作画Fを編集しているか分かる
- 作画とタイミング調整を行き来しやすい
- 出力結果がタイムシートと一致する
```

---

## 13. 危険な実装パターン

以下は禁止または避ける。

```text
- CellPanel内にTimesheetPanelを入れる
- UI描画中にcell.framesやtimesheet.entriesを増減する
- T選択時にactiveDrawingFrameIndexを暗黙に書き換える
- Project直下に仮Timesheetを増やす
- 表示反映と保存処理を同時に実装する
- 出力反映まで一気に実装する
- 存在しない作画F参照で落とす
```

---

## 14. MVPの完了条件

MVP完了条件は以下とする。

```text
1. 縦型TimesheetPanelが表示される
2. セル列とT行が表示される
3. 数字、ホールド、空セルを入力できる
4. 原画/中割を指定できる
5. T選択でキャンバス表示が変わる
6. どの作画Fを編集しているか表示される
7. 再生がタイムシート通りになる
8. PNG連番出力がタイムシート通りになる
9. MP4出力がタイムシート通りになる
10. フレーム追加、削除、彩色モード移行で落ちない
```

---

## 15. 将来拡張

将来的には以下を追加する。

```text
- セリフ欄と音声波形の同期
- カメラキーフレーム編集
- 撮影指示から撮影モードへの接続
- 背景美術管理との接続
- BOOK素材管理との接続
- 絵コンテ/レイアウトとの接続
- 口パク/目パチ補助
- 原画/中割の色分け
- 印刷用タイムシート出力
- CSV/JSONエクスポート
```

---

## 16. 現時点の決定事項

```text
- タイムシートは縦型UIにする
- 横方向はセル列にする
- 縦方向はタイムラインTにする
- 数字は作画F番号を意味する
- 作画FとタイムラインTは別管理にする
- タイムシートはCut単位にする
- CellPanelには戻さない
- 原画/中割は表示対象ではなく制作管理属性として扱う
- カメラ/撮影/セリフ欄は最初はテキスト欄として扱う
- TimesheetResolverをcore側に置く
- 実装はcore、io、UI、表示、出力の順で進める
```

---

## 17. 次に行う作業

次の実装作業は以下を推奨する。

```text
Timesheet Rebuild Step 1:
  core側にCut/Timesheetの最小モデルとTimesheetResolverを追加する。
```

Step 1でやること:

```text
- src/core/Cut.h を追加
- src/core/Timesheet.h を正式版として追加
- src/core/TimesheetResolver.h を追加
- 既存UIにはまだ接続しない
- 固定データで解決できる関数を作る
```

Step 1でやらないこと:

```text
- TimesheetPanel作成
- 保存読み込み接続
- キャンバス表示反映
- PNG/MP4出力反映
```

理由:

```text
データの意味と解決ルールが安定する前にUIを作ると、前回と同じくUI、保存、描画が混ざって壊れやすくなるため。
```
