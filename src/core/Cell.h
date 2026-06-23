#pragma once

// このファイルの役割:
// 撮影素材単位の「セル」を表す。
// キャラ・背景・エフェクトなどを別セルとして持ち、各セルが独立したフレーム列を持つ。

#include <string>
#include <vector>

#include "core/Frame.h"

namespace perapera {

struct CellPlacement {
    float x = 0.0f;
    float y = 0.0f;
    float scale = 1.0f;
    float rotation = 0.0f;
};

struct CellMotionKey {
    int frame = 0;
    CellPlacement placement;
    std::string interpolation = "linear";
};

struct Cell {
    std::string id = "cell_001";
    std::string name = "キャラA";
    std::string category = "character";
    int widthPx = 1920;
    int heightPx = 1080;
    int zOrder = 0;
    bool visible = true;
    float opacity = 1.0f;
    CellPlacement placement;
    std::vector<CellMotionKey> motionKeys;
    std::vector<Frame> frames;

    static Cell createDefault();

    Frame* frameOrNull(int frameIndex);
    const Frame* frameOrNull(int frameIndex) const;
};

} // namespace perapera
