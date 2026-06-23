// このファイルの役割:
// Layer の初期値生成を実装する。

#include "core/Layer.h"

#include <iomanip>
#include <sstream>

namespace perapera {

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
    return layer;
}

} // namespace perapera
