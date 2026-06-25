## 2026-06-25 Phase 1.5 Step 17: 彩色モード追加

### 作業概要
- final_spec_v6 の矛盾修正版を反映し、Phase 1.5 の次工程として AppMode::Coloring を追加した。
- 作画モードの描画処理を流用し、別の巨大な彩色用描画関数は作らない方針にした。

### 変更ファイル
- src/ui/App.h
- src/ui/App.cpp
- src/ui/AppDrawingMode.cpp
- src/ui/AppInput.cpp
- src/render/CanvasRenderer.h
- src/render/CanvasRenderer.cpp

### 実装内容
- モードタブに「3.5 彩色」を追加。
- 彩色モードへ入ったとき、Paint レイヤーを自動選択する。
- Paint レイヤーが存在しない場合は自動作成する。
- 彩色モードのデフォルトツールを FloodFill にする。
- 彩色モードでは Normal / ColorTrace / Paint / ShadowGuide / Rough の表示不透明度を用途別に変更する。
- 彩色モードのキャンバス背景を白にする。
- 彩色モードでは入力開始時に Paint レイヤーへ寄せ、参照レイヤーを直接編集しないようにした。

### 未完了
- Composite 書き出し時の ColorTrace 隣接 Paint 色置換は Step 18 で実装する。
- 本コンテナでは Windows / SDL3 / vcpkg / libmypaint 実ビルドは未実行。

### 次にやること
- Phase 1.5 Step 18: ColorTrace の最終出力処理。

### 判断待ち
- なし。
