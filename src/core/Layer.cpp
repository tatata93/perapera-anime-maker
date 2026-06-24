// このファイルの役割:
// Layer の初期値生成と LayerType の文字列変換を実装する。

#include "core/Layer.h"

#include <iomanip>
#include <sstream>

namespace perapera {

const char* layerTypeToString(LayerType type)
{
    switch (type) {
    case LayerType::Normal:
        return "Normal";
    case LayerType::ColorTrace:
        return "ColorTrace";
    case LayerType::Paint:
        return "Paint";
    case LayerType::ShadowGuide:
        return "ShadowGuide";
    case LayerType::Rough:
        return "Rough";
    }
    return "Normal";
}

const char* layerTypeDisplayName(LayerType type)
{
    switch (type) {
    case LayerType::Normal:
        return "Normal / 線画";
    case LayerType::ColorTrace:
        return "ColorTrace / 色トレス";
    case LayerType::Paint:
        return "Paint / 彩色";
    case LayerType::ShadowGuide:
        return "ShadowGuide / 影指定";
    case LayerType::Rough:
        return "Rough / ラフ";
    }
    return "Normal / 線画";
}

LayerType layerTypeFromString(std::string_view value)
{
    if (value == "ColorTrace" || value == "color_trace" || value == "色トレス") {
        return LayerType::ColorTrace;
    }
    if (value == "Paint" || value == "paint" || value == "彩色") {
        return LayerType::Paint;
    }
    if (value == "ShadowGuide" || value == "shadow_guide" || value == "影指定") {
        return LayerType::ShadowGuide;
    }
    if (value == "Rough" || value == "rough" || value == "ラフ") {
        return LayerType::Rough;
    }
    return LayerType::Normal;
}

Layer Layer::createDefault(int layerNumber)
{
    if (layerNumber < 1) {
        layerNumber = 1;
    }

    std::ostringstream idStream;
    idStream << "layer_" << std::setw(3) << std::setfill('0') << layerNumber;

    Layer layer;
    layer.layerId = idStream.str();
    layer.name = "線画";
    layer.visible = true;
    layer.opacity = 1.0f;
    layer.blendMode = "normal";
    layer.type = LayerType::Normal;
    return layer;
}

} // namespace perapera
