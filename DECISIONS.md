# DECISIONS

## Decision 001: 完全新規リポジトリとして開始
日付: 2026-06-23

理由:
既存コードの描画経路が複雑化し、仕様書 v3.0 で「既存コードは一切引き継がない」と定めたため。

影響:
Phase 0 から順番に、C++20 / CMake / SDL3 / Dear ImGui を土台として再構築する。

## Decision 002: libmypaintは任意検出のスタブから導入する
日付: 2026-06-25

理由:
Phase 1.5ではlibmypaintを導入するが、vcpkg環境が未設定の状態で既存の作画・保存・出力を壊さないため。
まずMyPaintBrushEngineのクラスとCMake検出だけを追加し、libmypaintが無い環境ではSimple互換フォールバックでビルドを通す。

影響:
Step 12ではブラシエンジン切り替えUIとスタブだけが追加される。
実際のdraw_dab接続、MyPaintブラシ設定読み込み、水彩表現はStep 13以降で追加する。
