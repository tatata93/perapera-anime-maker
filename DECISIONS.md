## Decision 018: Composite出力時だけColorTrace線をPaint色へ置換する
日付: 2026-06-25
理由: v6仕様では、ColorTraceの最終出力処理はCompositeモード内で行い、LineTest/ColorTrace/LineOnlyの書き出しモードは既存仕様を維持するため。
影響: 完成出力では色トレス線が近傍Paint色へ馴染む。Paint色がない箇所は確認しやすさを優先して元のColorTrace色を保持する。

## Decision 019: 作画キャンバス背景を白に統一する
日付: 2026-06-25
理由: 線画作業時も紙面に近い白背景で確認したいという要望に対応するため。
影響: アプリ全体のダークテーマは維持し、キャンバス描画領域のみ白背景になる。

## Decision 020: PNG書き出しはCanvasBitmapのCPU側RGBAを読む
日付: 2026-06-25
理由: PNG/MP4書き出しではSDL_Textureではなく、MyPaint再生後のCanvasBitmap上のCPU側RGBAを読む必要があるため。
影響: CanvasBitmapに読み取り専用アクセサを追加するだけで、描画・GPU転送・dirty管理の既存挙動は変えない。
