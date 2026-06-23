#pragma once

// このファイルの役割:
// 1フレーム内の1枚の作画レイヤーを表す。
// レイヤーはストローク集合を持つが、SDL_Textureなどの描画キャッシュは持たない。

#include <string>
#include <vector>

#include "core/Stroke.h"

namespace perapera {

struct Layer {
    std::string layerId = "layer_001";
    std::string name = "線画";
    bool visible = true;
    float opacity = 1.0f;
    std::string blendMode = "normal";
    std::vector<Stroke> strokes;

    static Layer createDefault(int layerNumber);
};

} // namespace perapera
