#pragma once

// このファイルの役割:
// バケツ塗りのアルゴリズムだけを定義する。
// UIやImGuiを知らない純粋な処理にして、Paintレイヤーへ追加する塗りストロークを返す。

#include <array>
#include <string>
#include <vector>

#include "core/Frame.h"
#include "core/Stroke.h"

namespace perapera::fill {

struct FloodFillSettings {
    // 0なら濃い線だけを壁にする。255に近いほど薄い線も壁として扱う。
    int tolerance = 24;

    // 小さな隙間を閉じるため、壁マスクをこの半径pxだけ膨らませる。
    int gapClosePx = 0;
};

struct FloodFillResult {
    bool success = false;
    std::string message;
    int filledPixelCount = 0;
    std::vector<Stroke> strokes;
};

FloodFillResult makeFloodFillStrokes(const Frame& frame,
                                      int targetLayerIndex,
                                      int canvasWidth,
                                      int canvasHeight,
                                      int seedX,
                                      int seedY,
                                      const std::array<float, 4>& fillColor,
                                      const FloodFillSettings& settings);

} // namespace perapera::fill
