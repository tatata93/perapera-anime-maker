// このファイルの役割:
// Frame の初期値生成と安全なレイヤー参照を実装する。

#include "core/Frame.h"

#include <iomanip>
#include <sstream>

namespace perapera {

Frame Frame::createDefault(int frameNumber)
{
    if (frameNumber < 1) {
        frameNumber = 1;
    }

    std::ostringstream nameStream;
    nameStream << "F" << std::setw(3) << std::setfill('0') << frameNumber;

    Frame frame;
    frame.name = nameStream.str();
    frame.durationFrames = 1;
    frame.layers.push_back(Layer::createDefault(1));
    return frame;
}

Layer* Frame::activeLayerOrNull(int layerIndex)
{
    if (layerIndex < 0 || layerIndex >= static_cast<int>(layers.size())) {
        return nullptr;
    }
    return &layers[static_cast<std::size_t>(layerIndex)];
}

const Layer* Frame::activeLayerOrNull(int layerIndex) const
{
    if (layerIndex < 0 || layerIndex >= static_cast<int>(layers.size())) {
        return nullptr;
    }
    return &layers[static_cast<std::size_t>(layerIndex)];
}

} // namespace perapera
