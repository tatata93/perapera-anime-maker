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

    // 境界の下までPaintを少し潜り込ませる半径px。
    // 旧実装では内側へ縮めていたが、白い隙間の原因になったため、
    // 現在はNormal/ColorTrace線の下へ塗りを伸ばす量として使う。
    int insetPx = 0;

    // 閉じていない領域で塗りが画面全体へ流れた時の安全弁。
    // 0なら無効。45ならキャンバスの45%以上を塗ろうとした時点で中止する。
    int leakGuardPercent = 45;
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
