# Phase 1.5 Step 2: LayerType foundation

## 変更
- `LayerType` を `Layer` に追加。
- `Normal / Rough / ColorTrace / Paint / ShadowGuide` を定義。
- `layer_NNN.json` に `type` を保存・読み込み。
- レイヤーパネルで選択中レイヤーの種別を変更できるようにした。

## 範囲
- 今回はデータモデルとUI土台のみ。
- 色トレス出力時消去、バケツ塗り、影指定の非出力化は次以降。

## 確認
- 既存プロジェクトは `type` が無ければ `Normal` として読む。
- 新規保存では `type` が layer JSON に出る。
