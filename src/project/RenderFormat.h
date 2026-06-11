// src/project/RenderFormat.h
//
// RenderFormatは「最終的に動画として出力する形式」を表すデータです。
//
// 重要:
// WorkCanvasは広い作画用の紙。
// RenderFormatは、その中から撮影で切り出す最終映像サイズです。

#pragma once

namespace perapera
{
    class RenderFormat
    {
    public:
        // 最終的に書き出す映像の横幅。
        int outputWidthPx = 1920;

        // 最終的に書き出す映像の高さ。
        int outputHeightPx = 1080;

        // 1秒あたりのフレーム数。
        int framesPerSecond = 24;

        // ピクセルの縦横比。
        // 現代の一般的な映像では1.0でよい。
        float pixelAspectRatio = 1.0f;

        // 値を安全な範囲に補正する。
        void clampToReasonableValues();

        // 出力映像の見た目上のアスペクト比を返す。
        // pixelAspectRatioも考慮する。
        float displayAspectRatio() const;
    };
}