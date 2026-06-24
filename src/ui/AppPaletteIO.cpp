// このファイルの役割:
// ColorPanel のスウォッチを palettes/palette.json に保存・読み込みする。
// プロジェクト本体の JSON 保存とは分け、色管理だけの責務にする。

#include "ui/App.h"

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace perapera {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

std::filesystem::path palettePathForProject(const std::filesystem::path& projectFolder)
{
    return projectFolder / "palettes" / "palette.json";
}

std::array<float, 4> readColorArray(const nlohmann::json& jsonColor,
                                    const std::array<float, 4>& fallback)
{
    if (!jsonColor.is_array() || jsonColor.size() < 4U) {
        return fallback;
    }

    std::array<float, 4> color = fallback;
    for (std::size_t i = 0; i < color.size(); ++i) {
        if (jsonColor[i].is_number()) {
            color[i] = std::clamp(jsonColor[i].get<float>(), 0.0f, 1.0f);
        }
    }
    return color;
}

nlohmann::json writeColorArray(const std::array<float, 4>& color)
{
    return nlohmann::json::array({color[0], color[1], color[2], color[3]});
}

ui::ColorSwatch readSwatch(const nlohmann::json& jsonSwatch)
{
    ui::ColorSwatch swatch;
    swatch.name = jsonSwatch.value("name", std::string("color"));
    swatch.group = jsonSwatch.value("group", std::string("basic"));
    swatch.color = readColorArray(jsonSwatch.value("color", nlohmann::json::array()),
                                  {0.05f, 0.05f, 0.05f, 1.0f});
    return swatch;
}

nlohmann::json writeSwatch(const ui::ColorSwatch& swatch)
{
    return nlohmann::json{
        {"name", swatch.name},
        {"group", swatch.group},
        {"color", writeColorArray(swatch.color)},
    };
}

std::vector<ui::ColorSwatch> readSwatchList(const nlohmann::json& jsonList)
{
    std::vector<ui::ColorSwatch> swatches;
    if (!jsonList.is_array()) {
        return swatches;
    }

    for (const nlohmann::json& item : jsonList) {
        if (item.is_object()) {
            swatches.push_back(readSwatch(item));
        }
    }
    return swatches;
}

nlohmann::json writeSwatchList(const std::vector<ui::ColorSwatch>& swatches)
{
    nlohmann::json jsonList = nlohmann::json::array();
    for (const ui::ColorSwatch& swatch : swatches) {
        jsonList.push_back(writeSwatch(swatch));
    }
    return jsonList;
}

void setError(std::string* error, const std::string& text)
{
    if (error != nullptr) {
        *error = text;
    }
}

} // namespace

bool App::saveColorPalette(const std::filesystem::path& projectFolder, std::string* error) const
{
    const std::filesystem::path palettePath = palettePathForProject(projectFolder);
    std::error_code errorCode;
    std::filesystem::create_directories(palettePath.parent_path(), errorCode);
    if (errorCode) {
        setError(error, "palette directory create failed: " + errorCode.message());
        return false;
    }

    nlohmann::json jsonPalette;
    jsonPalette["version"] = 1;
    jsonPalette["brushColor"] = writeColorArray(brushSettings_.color);
    jsonPalette["editColor"] = writeColorArray(colorPanelState_.editColor);
    jsonPalette["selectedSwatchIndex"] = colorPanelState_.selectedSwatchIndex;
    jsonPalette["swatches"] = writeSwatchList(colorPanelState_.swatches);
    jsonPalette["recentColors"] = writeSwatchList(colorPanelState_.recentColors);

    std::ofstream file(palettePath, std::ios::binary | std::ios::trunc);
    if (!file) {
        setError(error, "palette open failed: " + palettePath.string());
        return false;
    }
    file << std::setw(2) << jsonPalette << '\n';
    if (!file) {
        setError(error, "palette write failed: " + palettePath.string());
        return false;
    }
    return true;
}

bool App::loadColorPalette(const std::filesystem::path& projectFolder, std::string* error)
{
    const std::filesystem::path palettePath = palettePathForProject(projectFolder);
    if (!std::filesystem::exists(palettePath)) {
        colorPanelState_.initialized = false;
        colorPanelState_.paletteDirty = false;
        colorPanelState_.paletteStatus = u8c(u8"palette.json がありません。デフォルトパレットを使います。");
        return true;
    }

    std::ifstream file(palettePath, std::ios::binary);
    if (!file) {
        setError(error, "palette open failed: " + palettePath.string());
        return false;
    }

    nlohmann::json jsonPalette;
    try {
        file >> jsonPalette;
    } catch (const std::exception& exception) {
        setError(error, std::string("palette parse failed: ") + exception.what());
        return false;
    }

    if (!jsonPalette.is_object()) {
        setError(error, "palette root must be object: " + palettePath.string());
        return false;
    }

    ui::ColorPanelState loadedState;
    loadedState.initialized = true;
    loadedState.paletteDirty = false;
    loadedState.swatches = readSwatchList(jsonPalette.value("swatches", nlohmann::json::array()));
    loadedState.recentColors = readSwatchList(jsonPalette.value("recentColors", nlohmann::json::array()));
    loadedState.selectedSwatchIndex = jsonPalette.value("selectedSwatchIndex", -1);
    loadedState.editColor = readColorArray(jsonPalette.value("editColor", nlohmann::json::array()), brushSettings_.color);

    brushSettings_.color = readColorArray(jsonPalette.value("brushColor", nlohmann::json::array()),
                                          loadedState.editColor);

    if (loadedState.selectedSwatchIndex < -1 ||
        loadedState.selectedSwatchIndex >= static_cast<int>(loadedState.swatches.size())) {
        loadedState.selectedSwatchIndex = loadedState.swatches.empty() ? -1 : 0;
    }
    if (loadedState.swatches.empty()) {
        loadedState.initialized = false;
        loadedState.paletteStatus = u8c(u8"palette.json は空でした。デフォルトパレットを使います。");
    } else {
        loadedState.paletteStatus = u8c(u8"palette.json を読み込みました。");
    }

    colorPanelState_ = std::move(loadedState);
    return true;
}

} // namespace perapera
