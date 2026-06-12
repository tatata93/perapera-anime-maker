# DECISIONS

This file records important project decisions.

## Decision 001: Use C++20

### 理由

- 現代的なC++機能を使える。
- CMakeと組み合わせて複数環境でビルドしやすい。
- 将来的な大規模化に対応しやすい。

### 影響

- 古いコンパイラは対象外になる。
- Visual Studio 2022、最近のClang、最近のGCCを想定する。

## Decision 002: Do not use Qt in the initial phase

### 理由

- Qtは便利だが、LGPL/GPLモジュール管理に注意が必要。
- 初期段階ではライセンス安全寄りの構成を優先する。

### 影響

- 初期UIはSDL3 + Dear ImGuiを候補にする。
- 本格的な商用アプリ風UIは後回しにする。

## Decision 003: Do not integrate FFmpeg at Step 0

### 理由

- FFmpegはLGPL/GPL/nonfreeのビルド設定に注意が必要。
- 初期段階では動画コーデックを本体に組み込まない方が安全。

### 影響

- まずはPNG連番とWAV出力を優先する。
- MP4化は将来的に外部FFmpegをユーザー指定で呼び出す設計にする。

## Decision 004: Keep 3D as drawing guide only

### 理由

- このソフトの最終表現は手描き2Dアニメである。
- 3Dは完成映像ではなく、構図、パース、カメラ、動きの補助に使う。

### 影響

- 3Dガイドは最終レンダリングに出さない。
- 3Dモデルの写実レンダリングは初期実装しない。

## Decision 005: Use Japanese display name and ASCII technical names

### 決定

ソフトの正式な表示名は「ぺらぺらアニメ作り機」とする。

ただし、GitHubリポジトリ名、CMakeプロジェクト名、実行ファイル名は、環境差や文字化けを避けるためASCII名を使う。

```text
Names
├── Display name
│   └── ぺらぺらアニメ作り機
├── Repository name
│   └── perapera-anime-maker
├── CMake project name
│   └── PeraperaAnimeMaker
└── Executable name
    └── perapera_anime_maker
```

### 理由

- 日本語名を正式名称として扱いたい。
- しかし、ビルドツール、シェル、GitHub、実行ファイル名では日本語が原因で問題が出る可能性がある。
- そのため、ユーザーに見える名前と内部技術名を分ける。

### 影響

- READMEやUI表示では「ぺらぺらアニメ作り機」を使う。
- CMakeやGitHubでは `PeraperaAnimeMaker`、`perapera_anime_maker`、`perapera-anime-maker` を使う。

## Decision 007: Separate WorkCanvas and RenderFormat

### 決定

Phase 2では、作画キャンバスと最終出力形式を別のデータとして扱う。

```text
Phase2DataModel
├── WorkCanvas
│   └── 実際に絵を描く広い紙
└── RenderFormat
    └── 撮影で切り出して最終的に出力する映像形式
    ## Decision 008: Store early pen drawing as strokes

### 決定

Phase 3Aでは、簡易ペン描画を画像バッファではなくStrokeの配列として保存する。

```text
Phase3A_DrawingModel
├── Brush
│   ├── ペン半径
│   └── 色
├── Stroke
│   └── キャンバス座標上の点の列
└── DrawingCanvasPanel
    └── ImGui上でStrokeを仮描画する

    ## Decision 009: Add basic drawing layers before PNG export

### 決定

Phase 3Bでは、PNG保存より先に簡易レイヤー構造を追加する。

```text
Phase3B_LayerModel
├── DrawingLayer
│   ├── name
│   ├── visible
│   ├── opacity
│   └── strokes
└── DrawingCanvasPanel
    ├── activeLayerIndex
    ├── addLayer
    ├── deleteActiveLayer
    ├── moveActiveLayerUp
    └── moveActiveLayerDown

    ## stb_image_write

```text
Dependency
├── Name: stb_image_write.h
├── Purpose: PNG画像を書き出すため
├── Used in: src/export/PngExporter.cpp
├── License: Public domain / MIT style option
├── Added in: Phase 3C
└── Scope: 画像書き出しのみ

## Decision 010: Add PNG export using stb_image_write

### 決定

Phase 3Cでは、PNG保存のために `stb_image_write.h` を追加する。

```text
PNGExport
├── Input
│   ├── WorkCanvas
│   ├── RenderFormat
│   └── DrawingLayer[]
├── Process
│   ├── WorkCanvas中央の撮影フレームを切り出す
│   ├── 表示中レイヤーのみ合成する
│   ├── レイヤー不透明度を反映する
│   └── RGBA画像としてPNG保存する
└── Output
    └── exports/frame_0001.png
    ## Decision 011: Add frame management before timeline playback

### 決定

Phase 3Dでは、再生タイムラインより先に、複数フレームを持てるデータ構造を追加する。

```text
AnimationData
├── AnimationFrame[]
│   ├── name
│   ├── durationFrames
│   └── DrawingLayer[]
└── DrawingLayer[]
    └── Stroke[]

    ## Decision 012: Add onion skin before PNG sequence export

### 決定

Phase 3Eでは、PNG連番保存より先にオニオンスキン表示を追加する。

```text
OnionSkin
├── Previous frames
│   └── red/orange tint
├── Next frames
│   └── blue tint
├── Range
│   └── 1 to 3 frames
├── Opacity
│   └── adjustable
└── Layer visibility
    └── hidden layers can be excluded

    ## Decision 013: Add playback preview before PNG sequence export

### 決定

Phase 3Fでは、PNG連番保存より先に、アプリ内の再生プレビューを追加する。

```text
PlaybackPreview
├── Input
│   ├── RenderFormat.framesPerSecond
│   ├── AnimationFrame.durationFrames
│   └── AnimationFrame[]
├── State
│   ├── isPlaybackPlaying
│   ├── playbackLoopEnabled
│   ├── playbackSubFrameCounter
│   └── playbackTimeAccumulatorSeconds
└── Behavior
    ├── FPSに合わせてフレームを進める
    ├── durationFrames分だけ同じ絵を保持する
    ├── 最後まで行ったらループ可能
    └── 再生中は描画を止める