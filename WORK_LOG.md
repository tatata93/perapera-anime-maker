# WORK_LOG

This file records what has been done so that another AI, another developer, or another PC can understand the current state of the project.

## 2026-06-12

### 作業概要

- Step 0の初期プロジェクト構造を作成した。
- ソフト名を「ぺらぺらアニメ作り機」に決定した。
- 外部ライブラリはまだ追加していない。
- C++20とCMakeだけでビルドできる最小構成を作成した。

### 変更ファイル

```text
perapera-anime-maker/
├── docs/
│   ├── coding_rules.md
│   ├── roadmap.md
│   └── setup.md
├── src/
│   └── app/
│       └── main.cpp
├── .gitattributes
├── .gitignore
├── CMakeLists.txt
├── CMakePresets.json
├── DECISIONS.md
├── DEPENDENCIES.md
├── LICENSE
├── LICENSE_POLICY.md
├── README.md
└── WORK_LOG.md
```

### 実装内容

- `main.cpp` に最小のC++プログラムを作成した。
- `CMakeLists.txt` にC++20ビルド設定を追加した。
- `CMakePresets.json` に標準ビルドプリセットを追加した。
- Git LFS対象拡張子を `.gitattributes` に記録した。
- ライセンス方針と依存関係管理方針を文書化した。
- 日本語表示名とASCII技術名を分ける方針を記録した。

### 未完了

- SDL3は未追加。
- Dear ImGuiは未追加。
- Skiaは未追加。
- 作画キャンバスは未実装。
- カメラ機能は未実装。
- 3Dガイドは未実装。
- 簡易物理は未実装。
- 動画書き出しは未実装。

### 次にやること

- Phase 1として、SDL3とDear ImGuiを追加して最小ウィンドウを表示する。
- ただし、ライブラリ追加前に用途、ライセンス、無償公開時の注意点を確認する。

### 判断待ち

- 指示者がPhase 1に進むか判断する。
- MIT Licenseのままでよいか確認する。
## 2026-06-12

### 作業概要

- Phase 1として、SDL3とDear ImGuiを追加した。
- SDL3でウィンドウを作成し、Dear ImGuiで最小UIを表示する構成にした。

### 変更ファイル

```text
perapera-anime-maker/
├── CMakeLists.txt
├── DECISIONS.md
├── DEPENDENCIES.md
├── WORK_LOG.md
└── src/
    └── app/
        └── main.cpp

## 2026-06-12

### 作業概要

- Phase 2として、任意サイズ作画キャンバスと撮影フレームの土台を追加した。

### 変更ファイル

```text
perapera-anime-maker/
├── .gitignore
├── CMakeLists.txt
├── DECISIONS.md
├── WORK_LOG.md
└── src/
    ├── app/
    │   └── main.cpp
    ├── drawing/
    │   ├── WorkCanvas.h
    │   └── WorkCanvas.cpp
    ├── project/
    │   ├── RenderFormat.h
    │   └── RenderFormat.cpp
    └── ui/
        ├── CanvasPreview.h
        └── CanvasPreview.cpp
```

### 実装内容

- `WorkCanvas` を追加した。
- `RenderFormat` を追加した。
- `CanvasPreview` を追加した。
- ImGui上で作画キャンバスと撮影フレームを仮表示できるようにした。
- キャンバス設定パネルを追加した。
- 作画キャンバスサイズ、出力フレームサイズ、FPS、ピクセル縦横比を変更できるようにした。
- 撮影フレームが作画キャンバスより大きい場合に警告を出すようにした。
- `imgui.ini` をGit管理しないように `.gitignore` に追加した。

### 未完了

- ペン描画は未実装。
- レイヤーは未実装。
- オニオンスキンは未実装。
- カメラ移動は未実装。
- ズーム、ドリー、ドーリーズームは未実装。
- 背景画角キャリブレーションは未実装。
- 簡易物理は未実装。

### 次にやること

- Phase 3として、撮影フレームの切り出し処理または簡易作画キャンバスのどちらを先に作るか判断する。
- 候補A: 作画キャンバスに簡易ペン描画を追加する。
- 候補B: 撮影フレームの切り出しとPNG出力を先に作る。

### 判断待ち

- 次に、簡易ペン描画を先に作るか、撮影フレーム切り出しを先に作るか確認する。


## 2026-06-12

### 作業概要

- Phase 3Aとして、簡易ペン描画の土台を追加した。

### 変更ファイル

```text
perapera-anime-maker/
├── CMakeLists.txt
├── DECISIONS.md
├── WORK_LOG.md
└── src/
    ├── app/
    │   └── main.cpp
    ├── drawing/
    │   ├── Brush.h
    │   ├── Brush.cpp
    │   ├── Stroke.h
    │   └── Stroke.cpp
    └── ui/
        ├── DrawingCanvasPanel.h
        └── DrawingCanvasPanel.cpp

## 2026-06-12

### 作業概要

- Phase 3Bとして、簡易レイヤー構造を追加した。

### 変更ファイル

```text
perapera-anime-maker/
├── CMakeLists.txt
├── DECISIONS.md
├── WORK_LOG.md
└── src/
    ├── app/
    │   └── main.cpp
    ├── drawing/
    │   ├── DrawingLayer.h
    │   └── DrawingLayer.cpp
    └── ui/
        ├── DrawingCanvasPanel.h
        └── DrawingCanvasPanel.cpp

        