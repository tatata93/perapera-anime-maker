# WORK_LOG

## Phase 1 Step 1-4 stability pass v12

### 目的

再起動後に「読み込み」を押しても作画内容が見えない問題を修正する。
前回v11では保存/読み込み処理を分離したが、相対パスの基準と起動後の選択フレーム復元が弱かった。

### 原因

- `my_anime_project` のような相対プロジェクトパスが、起動時カレントディレクトリに依存していた。
  - VS Codeから起動した時と、exe位置/別のカレントディレクトリから起動した時で別フォルダを読みに行く危険があった。
- `ProjectIO` は作画データを保存するが、現在選択中のセル/フレーム/レイヤーは保存していなかった。
  - 再起動後に読み込んでも、空白フレームが選ばれていると「復元していない」ように見える。
- 保存後の即時検証が弱く、保存したストローク数と読み直したストローク数をステータスで確認しにくかった。

### 変更内容

- `src/ui/AppProjectIOSupport.h` を追加。
- `src/ui/AppProjectIOSupport.cpp` を追加。
- `CMakeLists.txt` に `src/ui/AppProjectIOSupport.cpp` を追加。
- 相対パスの基準を安定化した。
  - `CMakeLists.txt` と `src/` を持つリポジトリルートを上位探索し、相対パスの基準にする。
  - `my_anime_project` / `exports/png` / `exports/mp4/output.mp4` が起動方法でずれにくくなる。
- 保存時に `app_state.json` をプロジェクトフォルダへ書くようにした。
  - `activeCellIndex`
  - `activeFrameIndex`
  - `activeLayerIndex`
  - 保存時の統計値
- 読み込み時に `app_state.json` の選択状態を復元するようにした。
- 保存時に `ProjectIO::save()` 後、即 `ProjectIO::load()` して統計と署名を照合するようにした。
- 読み込み後、選択フレームが空白で、別フレームに線がある場合は最初の非空フレームを表示するようにした。
- 右サイドバーの表示を `Step 1-4 stability pass v12` に更新した。

### 変更ファイル

- `CMakeLists.txt`
- `WORK_LOG.md`
- `src/ui/AppProjectIO.cpp`
- `src/ui/AppProjectIOSupport.h`
- `src/ui/AppProjectIOSupport.cpp`
- `src/ui/AppDrawingMode.cpp`

### 確認項目

- ビルドが通ること。
- ③ 作画で `Step 1-4 stability pass v12` が表示されること。
- 1フレーム目に線を描き、保存できること。
- ステータスに `project saved+verified` と `strokes=` が表示されること。
- アプリを閉じて再起動し、読み込み後に線が復元されること。
- `my_anime_project/app_state.json` が作られること。
