# Scene Reference / Background Policy v1

## 目的

将来、キャンバス背景に絵コンテ、レイアウト、背景原図、仮背景、完成背景を配置できるようにする。
ただし、これらをセル作画Fやタイムシートセル列に混ぜない。

## 基本方針

参照画像・背景画像は `Scene Plate` として扱う。
Scene Plate は、作画セルとは別の表示層である。

```text
Scene Plate
- 絵コンテ
- レイアウト
- 背景原図
- 仮背景
- 完成背景
- カメラガイド
- 安全フレーム
```

## タイムシートとの関係

タイムシートは、セルの時間割を管理する。
Scene Plate は、カット全体またはT範囲に対して表示される下敷き/背景である。

```text
Timesheet: キャラAのT1は作画F1を表示
Scene Plate: このカット全体に絵コンテ画像を30%で表示
```

この2つは別責務にする。

## 出力制御

Scene Plate には出力方針を持たせる。

```text
ReferenceOnly: 作画用下敷き。PNG/MP4には出さない
PreviewOnly: プレビューだけ。正式出力には出さない
RenderOutput: 完成背景/撮影素材。PNG/MP4に出す
```

## レイヤー順

初期の合成順は次の方向。

```text
1. ReferenceOnly storyboard/layout plates
2. RenderOutput background plates
3. animation cells resolved by timesheet
4. guides / camera overlays
```

出力時は ReferenceOnly と guides を除外できるようにする。

## 中割との関係

中割作画時は、背景/絵コンテプレートを表示したまま、前後原画もライトテーブル表示する。

```text
絵コンテ/レイアウト下敷き
前原画ライトテーブル
次原画ライトテーブル
編集中の中割
```

この順序により、絵コンテを見ながら中割を描ける。
