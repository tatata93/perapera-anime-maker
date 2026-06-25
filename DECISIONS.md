## Decision 018c: Composite出力でColorTraceを描かずPaint色に吸収する
日付: 2026-06-26
理由: Step18の実装では近傍Paint色が見つからない場合にColorTrace元色を保持していたため、PNG/MP4に赤・青・黄緑の線が残った。また、FloodFillの漏れ防止・insetにより線とPaint色の間に白い隙間が出た。
影響: Composite出力ではColorTraceレイヤーを直接合成しない。ColorTrace線とその周辺の白い隙間は近傍Paint色で埋める。LineTest / ColorTrace / LineOnly の書き出しモードは従来の仕様を維持する。
