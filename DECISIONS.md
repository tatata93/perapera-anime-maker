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

## Decision 013: MyPaintBrushEngineはストローク単位で保存する
日付: 2026-06-25
理由: CanvasRendererは保存済みストローク点列からレイヤーキャッシュを再構築するため、現在のUI設定だけで描画エンジンを判定すると、過去に描いた線の見た目が後から変わってしまう。
影響: `Stroke` に `StrokeBrushEngine` を追加し、JSONでは `brushEngine` として保存する。旧データはSimple扱いにして互換性を保つ。

## Decision 014: MyPaint描画が0dabの場合はSimple焼き込みへ退避する
日付: 2026-06-25
理由: libmypaintが検出済みでも、短いストロークや設定値・環境差でdraw_dabが1つも出ない場合、ドラッグ中のプレビューだけ見えて確定後に線が消える。作画ソフトとして「描いた線が消えない」ことを最優先にするため。
影響: ストロークの保存形式はMyPaintのまま維持するが、表示可能dabが0個の場合だけCanvasBitmapへの焼き込みをSimpleへ退避する。MyPaintらしい描き味調整は後続Stepで行う。
