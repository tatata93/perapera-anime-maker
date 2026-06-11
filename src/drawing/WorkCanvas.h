// src/drawing/WorkCanvas.h
//
// WorkCanvasは「実際に絵を描く広い紙」を表すデータです。
//
// 重要:
// 作画キャンバスのサイズと、最終的に動画として出力するサイズは別です。
// たとえば、6000x1080の横長背景を描き、撮影では1920x1080だけ切り出す、
// という使い方を想定します。

#pragma once

namespace perapera
{
    class WorkCanvas
    {
    public:
        // 初期値は4K相当の広いキャンバスにしておく。
        // 後でプロジェクト作成時にユーザーが自由に変更できるようにする。
        int widthPx = 3840;
        int heightPx = 2160;

        // 幅と高さを安全な範囲に補正する。
        // 0や負数、極端に大きい値を防ぐために使う。
        void clampToReasonableSize();

        // 横幅 ÷ 高さ の比率を返す。
        // 高さが0になると割り算できないため、安全に処理する。
        float aspectRatio() const;
    };
}