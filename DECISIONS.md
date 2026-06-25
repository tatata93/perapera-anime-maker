## Decision 017: 彩色モードは作画モードの描画処理を流用する
日付: 2026-06-25
理由: final_spec_v6 では、彩色モードはデータを変えず表示とツールを変えるだけで、drawDrawingMode() を流用し巨大な別関数を作らない方針になっているため。
影響: AppMode::Coloring は drawDrawingMode() を通り、CanvasRenderer の表示モードだけを切り替える。

## Decision 018: 彩色モードの編集対象は Paint レイヤーに寄せる
日付: 2026-06-25
理由: 彩色モードでは Normal / ColorTrace / ShadowGuide を参照表示し、Paint レイヤーを主編集対象にする仕様のため。
影響: 彩色モードに入ると Paint レイヤーを自動選択し、なければ作成する。
