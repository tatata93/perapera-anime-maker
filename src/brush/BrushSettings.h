#pragma once

// このファイルの役割:
// Phase 1.5以降で使うブラシ設定を1か所に集約する。
// まだlibmypaintは導入せず、SimpleBrushEngineのまま安全に設定UIを増やす。

#include <array>

namespace perapera::ui {

enum class ToolKind {
    Brush,
    Eraser,
    FloodFill,
};

enum class BrushEngineKind {
    Simple,
    MyPaint,
};

enum class BrushPreset {
    Pen,
    Pencil,
    Watercolor,
    Airbrush,
};

struct BrushSettings {
    ToolKind tool = ToolKind::Brush;
    BrushEngineKind engine = BrushEngineKind::Simple;
    BrushPreset preset = BrushPreset::Pen;

    float radiusPx = 4.0f;
    float opacity = 1.0f;
    float hardness = 1.0f;
    float spacing = 0.25f;

    // Phase 1.5の描き味調整用。0なら既存動作に近い。
    float stabilizer = 0.0f;
    float entryFade = 0.0f;
    float exitFade = 0.0f;
    float pressureToSize = 0.0f;
    float pressureToOpacity = 0.0f;

    // libmypaint導入後にMyPaintBrushEngineへ渡す予定のパラメータ。
    float watercolorBleed = 0.0f;
    float colorMix = 0.0f;
    float dryRate = 1.0f;

    // バケツ塗り用。
    // tolerance は壁判定の感度、gapClosePx は小さな隙間閉じの半径px。
    // fillInsetPx は塗り領域を境界から内側に少し縮め、線へ食い込む塗りを防ぐ。
    // leakGuardPercent は閉じていない領域をクリックした時の巨大塗りを止める安全弁。
    // 0以外なら、塗りがキャンバス端まで到達した場合も漏れ扱いで止める。
    int fillTolerance = 24;
    int fillGapClosePx = 0;
    int fillInsetPx = 1;
    int fillLeakGuardPercent = 45;

    std::array<float, 4> color = {0.05f, 0.05f, 0.05f, 1.0f};
};

const char* brushPresetLabel(BrushPreset preset);
void applyBrushPreset(BrushSettings& settings, BrushPreset preset);

} // namespace perapera::ui
