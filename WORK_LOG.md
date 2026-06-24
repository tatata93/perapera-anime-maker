# Phase 1.5 Step 3: Rough / ShadowGuide display-output control

## 変更
- `Rough` レイヤーは表示上の不透明度を最大50%に制限。
- `ShadowGuide` レイヤーはPNG/MP4用のPNG連番出力から除外。
- レイヤーパネルに「ラフのみ表示」「ラフを隠す」「全レイヤー表示」を追加。
- レイヤー種別変更時に `Rough` は50%、`ShadowGuide` は75%へ初期表示不透明度を寄せる。
- `CanvasRenderer` のキャッシュ判定に `LayerType` を含めた。

## 範囲
- 既存のNormal描画、保存、読み込み、消しゴム、PNG/MP4出力の入口は維持。
- バケツ塗り、色トレス線の出力時消去、libmypaint導入は未実装。

## 確認
- Roughにしたレイヤーが半透明で見える。
- ShadowGuideにしたレイヤーは作画画面では見えるが、書き出しPNG/MP4には入らない。
