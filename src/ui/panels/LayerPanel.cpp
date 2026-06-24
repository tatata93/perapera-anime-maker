// このファイルの役割:
// レイヤーパネルの実装。
// レイヤー種別を編集できるようにし、Phase 1.5 のラフ表示・影指定非出力の土台を作る。

#include "ui/panels/LayerPanel.h"

#include <algorithm>
#include <cstddef>

#include <imgui.h>

namespace perapera::ui {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

constexpr LayerType kLayerTypes[] = {
    LayerType::Normal,
    LayerType::Rough,
    LayerType::ColorTrace,
    LayerType::Paint,
    LayerType::ShadowGuide,
};

void applyLayerTypeDefaults(Layer& layer, LayerType previousType)
{
    if (previousType == layer.type) {
        return;
    }

    if (layer.type == LayerType::Rough) {
        layer.opacity = std::min(layer.opacity, 0.50f);
        return;
    }

    if (layer.type == LayerType::ShadowGuide) {
        layer.opacity = std::min(layer.opacity, 0.75f);
        return;
    }

    if ((previousType == LayerType::Rough || previousType == LayerType::ShadowGuide) && layer.opacity < 0.76f) {
        layer.opacity = 1.0f;
    }
}

void showOnlyRoughLayers(Frame& frame)
{
    for (Layer& layer : frame.layers) {
        layer.visible = layer.type == LayerType::Rough;
    }
}

void hideRoughLayers(Frame& frame)
{
    for (Layer& layer : frame.layers) {
        if (layer.type == LayerType::Rough) {
            layer.visible = false;
        }
    }
}

void showAllLayers(Frame& frame)
{
    for (Layer& layer : frame.layers) {
        layer.visible = true;
    }
}

void drawLayerTypeCombo(Layer& layer)
{
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("##LayerType_v27", layerTypeDisplayName(layer.type))) {
        for (LayerType type : kLayerTypes) {
            const bool selected = layer.type == type;
            if (ImGui::Selectable(layerTypeDisplayName(type), selected)) {
                const LayerType previousType = layer.type;
                layer.type = type;
                applyLayerTypeDefaults(layer, previousType);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

} // namespace

LayerPanelAction drawLayerPanel(Frame& frame, int& activeLayerIndex)
{
    ImGui::PushID("LayerPanel_v27_rough_shadow_output");

    ImGui::TextUnformatted(u8c(u8"レイヤー"));
    ImGui::Text("count: %d", static_cast<int>(frame.layers.size()));

    LayerPanelAction action = LayerPanelAction::None;

    if (ImGui::Button(u8c(u8"レイヤー追加##LayerPanel_AddLayer_v27"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::AddLayer;
    }

    const bool canDelete = frame.layers.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(u8c(u8"レイヤー削除##LayerPanel_DeleteLayer_v27"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::DeleteLayer;
    }
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    if (ImGui::Button(u8c(u8"レイヤークリア##LayerPanel_ClearLayer_v27"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::ClearLayer;
    }

    if (frame.layers.empty()) {
        ImGui::TextDisabled("no layers");
        ImGui::PopID();
        return action;
    }

    activeLayerIndex = std::clamp(activeLayerIndex, 0, static_cast<int>(frame.layers.size()) - 1);

    ImGui::Separator();
    ImGui::TextUnformatted(u8c(u8"選択中レイヤー"));
    Layer& activeLayer = frame.layers[static_cast<std::size_t>(activeLayerIndex)];
    drawLayerTypeCombo(activeLayer);
    ImGui::TextDisabled("json type: %s", layerTypeToString(activeLayer.type));
    ImGui::TextDisabled(u8c(u8"Roughは表示上50%%上限 / ShadowGuideはPNG・MP4出力に含めない"));

    ImGui::Separator();
    ImGui::TextUnformatted(u8c(u8"ラフ表示"));
    if (ImGui::Button(u8c(u8"ラフのみ表示##LayerPanel_ShowOnlyRough_v27"), ImVec2(-1.0f, 0.0f))) {
        showOnlyRoughLayers(frame);
    }
    if (ImGui::Button(u8c(u8"ラフを隠す##LayerPanel_HideRough_v27"), ImVec2(-1.0f, 0.0f))) {
        hideRoughLayers(frame);
    }
    if (ImGui::Button(u8c(u8"全レイヤー表示##LayerPanel_ShowAll_v27"), ImVec2(-1.0f, 0.0f))) {
        showAllLayers(frame);
    }

    ImGui::Separator();
    ImGui::TextUnformatted(u8c(u8"レイヤー一覧"));
    for (int index = static_cast<int>(frame.layers.size()) - 1; index >= 0; --index) {
        Layer& layer = frame.layers[static_cast<std::size_t>(index)];
        ImGui::PushID(index);
        ImGui::Checkbox("##LayerVisible_v27", &layer.visible);
        ImGui::SameLine();
        const bool selected = activeLayerIndex == index;
        if (ImGui::Selectable(layer.name.c_str(), selected)) {
            activeLayerIndex = index;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("[%s]", layerTypeToString(layer.type));
        ImGui::SetNextItemWidth(80.0f);
        ImGui::SliderFloat("##LayerOpacity_v27", &layer.opacity, 0.0f, 1.0f, "%.2f");
        ImGui::PopID();
    }

    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
