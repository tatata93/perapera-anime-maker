#include "io/LayerLayoutIO.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "core/Stroke.h"

namespace perapera {
namespace {

using nlohmann::json;

std::string numberedId(const char* prefix, int number) {
    if (number < 1) {
        number = 1;
    }
    std::ostringstream stream;
    stream << prefix << std::setw(3) << std::setfill('0') << number;
    return stream.str();
}

json strokePointToJson(const StrokePoint& point) {
    return json{{"x", point.x}, {"y", point.y}, {"pressure", point.pressure}};
}

json strokeToJson(const Stroke& stroke) {
    json result;
    result["brushEngine"] = strokeBrushEngineToString(stroke.brushEngine);
    result["color"] = stroke.color;
    result["radiusPx"] = stroke.radiusPx;
    result["opacity"] = stroke.opacity;
    result["hardness"] = stroke.hardness;
    result["spacing"] = stroke.spacing;
    result["pressureToSize"] = stroke.pressureToSize;
    result["pressureToOpacity"] = stroke.pressureToOpacity;
    result["watercolorBleed"] = stroke.watercolorBleed;
    result["colorMix"] = stroke.colorMix;
    result["dryRate"] = stroke.dryRate;

    json points = json::array();
    for (const StrokePoint& point : stroke.points) {
        points.push_back(strokePointToJson(point));
    }
    result["points"] = std::move(points);

    if (stroke.brushEngine == StrokeBrushEngine::Fill) {
        result["bitmap"] = json{{"x", stroke.bitmapX},
                                 {"y", stroke.bitmapY},
                                 {"width", stroke.bitmapWidth},
                                 {"height", stroke.bitmapHeight},
                                 {"data", stroke.bitmap}};
    }

    return result;
}

json layerToJson(const Layer& layer) {
    json result;
    result["schema"] = "perapera.layer.v1";
    result["layerId"] = layer.layerId;
    result["name"] = layer.name;
    result["visible"] = layer.visible;
    result["opacity"] = layer.opacity;
    result["blendMode"] = layer.blendMode;
    result["type"] = layerTypeToString(layer.type);

    json strokes = json::array();
    for (const Stroke& stroke : layer.strokes) {
        strokes.push_back(strokeToJson(stroke));
    }
    result["strokes"] = std::move(strokes);
    return result;
}

bool writeJsonFile(const std::filesystem::path& filePath, const json& data) {
    try {
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }
        std::ofstream output(filePath, std::ios::binary);
        if (!output) {
            return false;
        }
        output << data.dump(2) << '\n';
        return static_cast<bool>(output);
    } catch (...) {
        return false;
    }
}

} // namespace

bool saveLayerLayoutJson(const std::filesystem::path& layerJsonPath, const Layer& layer) {
    return writeJsonFile(layerJsonPath, layerToJson(layer));
}

bool saveFrameLayersLayout(const std::filesystem::path& frameDirectory, const Frame& frame) {
    try {
        const std::filesystem::path layersDirectory = frameDirectory / "layers";
        std::filesystem::create_directories(layersDirectory);

        int layerNumber = 1;
        for (const Layer& sourceLayer : frame.layers) {
            Layer layer = sourceLayer;
            if (layer.layerId.empty()) {
                layer.layerId = numberedId("layer_", layerNumber);
            }
            const std::filesystem::path layerJsonPath = layersDirectory / (layer.layerId + ".json");
            if (!saveLayerLayoutJson(layerJsonPath, layer)) {
                return false;
            }
            ++layerNumber;
        }
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace perapera
