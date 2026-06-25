#pragma once

// このファイルの役割:
// Phase 1.5以降で使うブラシ設定を1か所に集約する。
// Step 12でMyPaintBrushEngineスタブを追加する。
// 実際のlibmypaint描画接続は次Step以降で行い、SimpleBrushEngineは安全な保険として残す。

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

    // MyPaintBrushEngineへ渡すパラメータ。
    // Step 14以降はStrokeへ保存し、保存/読み込み後も描き味を復元する。
    float watercolorBleed = 0.0f;
    float colorMix = 0.0f;
    float dryRate = 1.0f;

    // バケツ塗り用。
    // tolerance は壁判定の感度、gapClosePx は小さな隙間閉じの半径px。
    // fillInsetPx は境界下塗り量。Normal/ColorTrace線の半透明エッジ下へPaintを伸ばし、白い1px隙間を防ぐ。
    // Step 18i以降は、外側へ越えないように外側到達マスクで制限する。
    // leakGuardPercent は閉じていない領域をクリックした時の巨大塗りを止める安全弁。
    // 0なら安全弁OFF。0以外なら、塗り面積がキャンバス全体の指定%を超えた時だけ止める。
    int fillTolerance = 24;
    int fillGapClosePx = 1;
    int fillInsetPx = 5;
    int fillLeakGuardPercent = 45;

    std::array<float, 4> color = {0.05f, 0.05f, 0.05f, 1.0f};
};

const char* brushEngineKindLabel(BrushEngineKind kind);
const char* brushPresetLabel(BrushPreset preset);
void applyBrushPreset(BrushSettings& settings, BrushPreset preset);

} // namespace perapera::ui
