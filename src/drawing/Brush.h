// src/drawing/Brush.h
//
// Brushは、作画に使うペンの設定を表すデータです。
// Phase 3Aでは、ペンの太さと色だけを持ちます。
// 筆圧、入り抜き、ブラシ形状、テクスチャなどは後の段階で追加します。

#pragma once

namespace perapera
{
    // RGBA形式の色。
    // r, g, b, a は 0.0〜1.0 の範囲で扱う。
    struct ColorRgba
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    class Brush
    {
    public:
        // ペンの半径。
        // 実際に線を描くときの太さは、おおむね radiusPx * 2 になる。
        float radiusPx = 4.0f;

        // ペンの色。
        // 初期値は白。
        ColorRgba color;
    };
}