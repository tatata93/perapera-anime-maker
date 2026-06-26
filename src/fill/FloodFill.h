#pragma once

// このファイルの役割:
// バケツ塗りのアルゴリズムだけを定義する。
// UIやImGuiを知らない純粋な処理にして、Paintレイヤーへ追加する塗りストロークを返す。
// Phase 1.5 Step 18g: ColorTrace線の半透明エッジ下までPaintを潜り込ませる。
// Phase 1.5 Step 18h: 1px白フチ対策としてPaintマスクの最終シールを追加。
// Phase 1.5 Step 18i: MyPaint線の隙間と細い線のはみ出しを分けて抑える。
// Phase 1.5 Step 18j: 境界近傍はドット、内部は横スパンで保存して、隙間とはみ出しを両立して抑える。
// Phase 1.5 Step 19: バケツ塗り結果をFillStrokeの1chマスクとして保存する。

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
    int gapClosePx = 1;

    // 境界の下までPaintを潜り込ませる半径px。
    // 旧実装では内側へ縮めていたが、白い隙間の原因になったため、
    // 現在はNormal/ColorTrace線の半透明エッジ下まで塗りを伸ばす量として使う。
    int insetPx = 5;

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
