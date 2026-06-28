# Timesheet Rebuild Step 3.5: TimesheetPanel 表示接続メモ

このStepでは、Step 3で追加した `TimesheetPanel` 表示専用スケルトンを作画モードから呼び出す。

## 方針

- まだ編集はしない。
- まだ保存・読み込みへ接続しない。
- まだキャンバス表示、再生、PNG/MP4出力へ反映しない。
- 作画FとタイムラインTはまだ連動させない。
- `CellPanel` には混ぜない。
- タイムシートは通常のDear ImGui独立ウィンドウとして表示する。

## 大ウィンドウ外ドラッグについて

現段階では、通常のImGuiウィンドウとしてソフト内で移動できる。
大ウィンドウ外へドラッグするには、Dear ImGui multi-viewports を有効化し、SDL3 + SDL_Renderer3構成で安定するか確認する必要がある。

そのため、このStepでは大ウィンドウ外ドラッグをまだ有効化しない。
次以降で `io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable` と SDL backend の挙動を検証してから対応する。

## なぜ差し込みスクリプト方式か

このStepでは `AppDrawingMode.cpp` に表示呼び出しを入れる必要がある。
ただし、ユーザー環境は `5dddc72` へ巻き戻した後に Step 1〜3 を適用済みであり、こちらの手元にある古い `a9cdb80` 由来のAppファイルで丸ごと上書きすると、仮タイムシートの古い実装を戻す危険がある。

そのため今回は例外的に、目印を確認してから最小差し込みだけを行うPowerShellスクリプトにした。
スクリプトは `AppDrawingMode.cpp` だけを変更し、変更前バックアップをデスクトップへ保存する。
