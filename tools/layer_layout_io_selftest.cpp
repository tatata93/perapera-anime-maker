#include <filesystem>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "core/Frame.h"
#include "core/Layer.h"
#include "core/Stroke.h"
#include "io/LayerLayoutIO.h"

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}

perapera::Stroke makeSimpleStroke() {
    perapera::Stroke stroke;
    stroke.brushEngine = perapera::StrokeBrushEngine::Simple;
    stroke.radiusPx = 6.0f;
    stroke.opacity = 0.85f;
    stroke.addPoint({10.0f, 20.0f, 1.0f});
    stroke.addPoint({30.0f, 40.0f, 0.5f});
    return stroke;
}

perapera::Stroke makeFillStroke() {
    perapera::Stroke stroke;
    stroke.brushEngine = perapera::StrokeBrushEngine::Fill;
    stroke.opacity = 1.0f;
    stroke.bitmapX = 2;
    stroke.bitmapY = 3;
    stroke.bitmapWidth = 2;
    stroke.bitmapHeight = 2;
    stroke.bitmap = {0, 255, 255, 0};
    return stroke;
}

} // namespace

int main() {
    namespace fs = std::filesystem;

    const fs::path root = fs::temp_directory_path() / "perapera_layer_layout_io_selftest";
    fs::remove_all(root);

    perapera::Frame frame;
    frame.name = "F001";
    frame.durationFrames = 1;

    perapera::Layer lineLayer;
    lineLayer.layerId = "layer_001";
    lineLayer.name = "Line";
    lineLayer.visible = true;
    lineLayer.opacity = 0.75f;
    lineLayer.blendMode = "normal";
    lineLayer.type = perapera::LayerType::Normal;
    lineLayer.strokes.push_back(makeSimpleStroke());
    lineLayer.strokes.push_back(makeFillStroke());
    frame.layers.push_back(lineLayer);

    perapera::Layer paintLayer;
    paintLayer.layerId = "layer_002";
    paintLayer.name = "Paint";
    paintLayer.type = perapera::LayerType::Paint;
    frame.layers.push_back(paintLayer);

    const fs::path frameDirectory = root / "scenes" / "scene_001" / "cuts" / "cut_001" /
                                    "cells" / "cell_A" / "frames" / "frame_001";

    if (!require(perapera::saveFrameLayersLayout(frameDirectory, frame), "saveFrameLayersLayout failed")) {
        return 1;
    }

    const fs::path layer001 = frameDirectory / "layers" / "layer_001.json";
    const fs::path layer002 = frameDirectory / "layers" / "layer_002.json";
    if (!require(fs::exists(layer001), "layer_001.json was not written")) {
        return 1;
    }
    if (!require(fs::exists(layer002), "layer_002.json was not written")) {
        return 1;
    }

    std::ifstream input(layer001);
    nlohmann::json json;
    input >> json;

    if (!require(json.value("schema", "") == "perapera.layer.v1", "wrong layer schema")) {
        return 1;
    }
    if (!require(json.value("layerId", "") == "layer_001", "wrong layer id")) {
        return 1;
    }
    if (!require(json.value("type", "") == "Normal", "wrong layer type")) {
        return 1;
    }
    if (!require(json.contains("strokes") && json["strokes"].size() == 2, "wrong stroke count")) {
        return 1;
    }
    if (!require(json["strokes"][0].value("brushEngine", "") == "Simple", "missing simple stroke")) {
        return 1;
    }
    if (!require(json["strokes"][1].value("brushEngine", "") == "Fill", "missing fill stroke")) {
        return 1;
    }
    if (!require(json["strokes"][1].contains("bitmap"), "fill bitmap was not saved")) {
        return 1;
    }

    fs::remove_all(root);
    std::cout << "perapera_layer_layout_io_selftest passed\n";
    return 0;
}
