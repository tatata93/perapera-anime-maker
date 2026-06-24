#pragma once

// このファイルの役割:
// 作画モード左サイドバーの色管理パネルを定義する。
// Phase 1.5 Step 6では、ブラシ色、スウォッチ、最近使った色をUI上で扱う。

#include <array>
#include <string>
#include <vector>

#include "brush/BrushSettings.h"

namespace perapera::ui {

struct ColorSwatch {
    std::string name;
    std::string group;
    std::array<float, 4> color = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct ColorPanelState {
    bool initialized = false;
    int selectedSwatchIndex = -1;
    std::array<float, 4> editColor = {0.05f, 0.05f, 0.05f, 1.0f};
    std::vector<ColorSwatch> swatches;
    std::vector<ColorSwatch> recentColors;
    char nameBuffer[64] = "color";
    char groupBuffer[64] = "basic";
};

void drawColorPanel(ColorPanelState& state, BrushSettings& brushSettings);

} // namespace perapera::ui
