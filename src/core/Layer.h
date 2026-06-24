#pragma once

// このファイルの役割:
// 1フレーム内の1枚の作画レイヤーを表す。
// レイヤーはストローク集合を持つが、SDL_Textureなどの描画キャッシュは持たない。

#include <string>
#include <string_view>
#include <vector>

#include "core/Stroke.h"

namespace perapera {

enum class LayerType {
    Normal,
    ColorTrace,
    Paint,
    ShadowGuide,
    Rough,
};

const char* layerTypeToString(LayerType type);
const char* layerTypeDisplayName(LayerType type);
LayerType layerTypeFromString(std::string_view value);

struct Layer {
    std::string layerId = "layer_001";
    std::string name = "線画";
    bool visible = true;
    float opacity = 1.0f;
    std::string blendMode = "normal";
    LayerType type = LayerType::Normal;
    std::vector<Stroke> strokes;

    static Layer createDefault(int layerNumber);
};

} // namespace perapera
