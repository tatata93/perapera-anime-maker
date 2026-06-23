#pragma once

// このファイルの役割:
// 作画モード左サイドバーのブラシ設定UIを定義する。

#include <array>

namespace perapera::ui {

enum class ToolKind {
    Brush,
    Eraser,
};

struct BrushSettings {
    ToolKind tool = ToolKind::Brush;
    float radiusPx = 4.0f;
    float opacity = 1.0f;
    std::array<float, 4> color = {0.05f, 0.05f, 0.05f, 1.0f};
};

void drawBrushPanel(BrushSettings& settings);

} // namespace perapera::ui
