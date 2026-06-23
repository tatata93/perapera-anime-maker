// このファイルの役割:
// レイヤーパネルの実装。
// パネル間で同じ表示名のボタンがあってもID衝突しないよう、PushIDで分離する。

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

} // namespace

LayerPanelAction drawLayerPanel(Frame& frame, int& activeLayerIndex)
{
    ImGui::PushID("LayerPanel_v4");
    ImGui::PushID(static_cast<const void*>(&frame));

    ImGui::TextUnformatted(u8c(u8"レイヤー"));
    ImGui::Text("count: %d", static_cast<int>(frame.layers.size()));

    LayerPanelAction action = LayerPanelAction::None;

    ImGui::PushID("AddLayerButton_v4");
    if (ImGui::Button(u8c(u8"レイヤー追加"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::AddLayer;
    }
    ImGui::PopID();

    const bool canDelete = frame.layers.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    ImGui::PushID("DeleteLayerButton_v4");
    if (ImGui::Button(u8c(u8"レイヤー削除"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::DeleteLayer;
    }
    ImGui::PopID();
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    ImGui::PushID("ClearLayerButton_v4");
    if (ImGui::Button(u8c(u8"レイヤークリア"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::ClearLayer;
    }
    ImGui::PopID();

    if (frame.layers.empty()) {
        ImGui::TextDisabled("no layers");
        ImGui::PopID();
        ImGui::PopID();
        return action;
    }

    activeLayerIndex = std::clamp(activeLayerIndex, 0, static_cast<int>(frame.layers.size()) - 1);
    for (int index = static_cast<int>(frame.layers.size()) - 1; index >= 0; --index) {
        Layer& layer = frame.layers[static_cast<std::size_t>(index)];
        ImGui::PushID(index);
        ImGui::Checkbox("##LayerVisible", &layer.visible);
        ImGui::SameLine();
        const bool selected = activeLayerIndex == index;
        if (ImGui::Selectable(layer.name.c_str(), selected)) {
            activeLayerIndex = index;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::SliderFloat("##LayerOpacity", &layer.opacity, 0.0f, 1.0f, "%.2f");
        ImGui::PopID();
    }

    ImGui::PopID();
    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
