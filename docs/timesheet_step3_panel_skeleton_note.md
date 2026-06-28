# Timesheet Rebuild Step 3: TimesheetPanel 表示専用スケルトン メモ

## 目的

正式タイムシートUIを `CellPanel` へ戻さず、独立した `TimesheetPanel` として作る。
Step 3では編集・保存・キャンバス反映を行わず、縦型タイムシート表と独立ウィンドウの土台だけを追加する。

## 今回の範囲

- `src/ui/panels/TimesheetPanel.h`
- `src/ui/panels/TimesheetPanel.cpp`
- `TimesheetPanelViewModel`
- `TimesheetPanelState`
- `TimesheetPanelResult`
- 表示専用の縦型テーブル
- T選択
- セリフ / カメラ / 撮影 / 素材メモ列の表示枠

## 今回接続しないもの

- `Project`
- `Cut`
- `TimesheetIO`
- `CutIO`
- キャンバス表示
- 再生
- PNG / MP4 出力

## ウィンドウ外ドラッグ方針

通常のImGuiウィンドウとして移動できる状態にする。
アプリ本体の大ウィンドウ外へドラッグ可能にするには、Dear ImGui multi-viewports を有効化する必要がある。

ただし、現在の構成は SDL3 + SDL_Renderer3 + ImGui なので、multi-viewports有効化時に以下を検証してから正式対応する。

- 追加プラットフォームウィンドウがSDL3で安定して生成されるか
- SDL_Renderer3バックエンドで複数viewport描画が安定するか
- DPI、フォーカス、IME、日本語入力が壊れないか
- メインループ末尾で `ImGui::UpdatePlatformWindows()` / `ImGui::RenderPlatformWindowsDefault()` を安全に呼べるか

Step 3ではmulti-viewportsはまだ有効化しない。

## 次の推奨作業

Step 3.5で、現在の `Project` のセル一覧から `TimesheetPanelViewModel` を組み立て、作画モードから表示だけ接続する。
その際も、まだ編集・保存・キャンバス反映は行わない。
