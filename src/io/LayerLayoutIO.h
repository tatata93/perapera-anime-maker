#pragma once

#include <filesystem>

#include "core/Frame.h"
#include "core/Layer.h"

namespace perapera {

bool saveLayerLayoutJson(const std::filesystem::path& layerJsonPath, const Layer& layer);
bool saveFrameLayersLayout(const std::filesystem::path& frameDirectory, const Frame& frame);

} // namespace perapera
