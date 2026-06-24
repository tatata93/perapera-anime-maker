## 2026-06-25 Phase 1.5 Step 13b: libmypaint検出修正

### 作業概要
- vcpkg manifest modeでlibmypaintはインストール済みなのに、CMake側でPERAPERA_HAS_LIBMYPAINTが定義されない問題を修正。
- CMake config / pkg-config に加えて、`vcpkg_installed/<triplet>` 直下のヘッダ・import libを直接検出するフォールバックを追加。

### 変更ファイル
- CMakeLists.txt

### 実装内容
- `vcpkg_installed/x64-windows/include/libmypaint/mypaint-brush.h` と `vcpkg_installed/x64-windows/lib/mypaint.lib` を直接検出。
- 検出できた場合は `PERAPERA_HAS_LIBMYPAINT=1` を定義。
- libmypaint / GLib系ヘッダのincludeディレクトリを追加。
- `mypaint.lib` を直接リンク。
- `vcpkg_installed/x64-windows/bin` のDLLをexe横へコピーするpost-build処理を追加。

### 未完了
- libmypaint APIの描き味調整は次Stepで継続。

### 次にやること
- MyPaintBrushEngine選択時に「libmypaint未検出」が消えるか確認。
- 線が描けるか確認。
