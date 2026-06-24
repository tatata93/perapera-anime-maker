// このファイルの役割:
// レイヤーパネルの実装。
// レイヤー種別を編集できるようにし、Phase 1.5 の色トレス・彩色の土台を作る。

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

void drawLayerTypeCombo(Layer& layer)
{
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("##LayerType_v26", layerTypeDisplayName(layer.type))) {
        for (LayerType type : kLayerTypes) {
            const bool selected = layer.type == type;
            if (ImGui::Selectable(layerTypeDisplayName(type), selected)) {
                layer.type = type;
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
    ImGui::PushID("LayerPanel_v26_layer_type");

    ImGui::TextUnformatted(u8c(u8"レイヤー"));
    ImGui::Text("count: %d", static_cast<int>(frame.layers.size()));

    LayerPanelAction action = LayerPanelAction::None;

    if (ImGui::Button(u8c(u8"レイヤー追加##LayerPanel_AddLayer_v26"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::AddLayer;
    }

    const bool canDelete = frame.layers.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(u8c(u8"レイヤー削除##LayerPanel_DeleteLayer_v26"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::DeleteLayer;
    }
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    if (ImGui::Button(u8c(u8"レイヤークリア##LayerPanel_ClearLayer_v26"), ImVec2(-1.0f, 0.0f))) {
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

    ImGui::Separator();
    ImGui::TextUnformatted(u8c(u8"レイヤー一覧"));
    for (int index = static_cast<int>(frame.layers.size()) - 1; index >= 0; --index) {
        Layer& layer = frame.layers[static_cast<std::size_t>(index)];
        ImGui::PushID(index);
        ImGui::Checkbox("##LayerVisible_v26", &layer.visible);
        ImGui::SameLine();
        const bool selected = activeLayerIndex == index;
        if (ImGui::Selectable(layer.name.c_str(), selected)) {
            activeLayerIndex = index;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("[%s]", layerTypeToString(layer.type));
        ImGui::SetNextItemWidth(80.0f);
        ImGui::SliderFloat("##LayerOpacity_v26", &layer.opacity, 0.0f, 1.0f, "%.2f");
        ImGui::PopID();
    }

    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
