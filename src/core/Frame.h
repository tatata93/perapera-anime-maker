#pragma once

// このファイルの役割:
// セル内の1コマを表す。
// 1コマは複数レイヤーを持ち、durationFrames でタイムライン上の尺を表す。

#include <string>
#include <vector>

#include "core/Layer.h"

namespace perapera {

struct Frame {
    std::string name = "F001";
    int durationFrames = 1;
    std::vector<Layer> layers;

    static Frame createDefault(int frameNumber);

    Layer* activeLayerOrNull(int layerIndex);
    const Layer* activeLayerOrNull(int layerIndex) const;
};

} // namespace perapera
