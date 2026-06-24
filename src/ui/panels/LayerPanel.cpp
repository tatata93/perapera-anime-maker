// このファイルの役割:
// レイヤーパネルの実装。
// パネル間で同じ表示名のボタンがあってもID衝突しないよう固定IDで分離する。

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
    ImGui::PushID("LayerPanel_v10_noptr");

    ImGui::TextUnformatted(u8c(u8"レイヤー"));
    ImGui::Text("count: %d", static_cast<int>(frame.layers.size()));

    LayerPanelAction action = LayerPanelAction::None;

    if (ImGui::Button(u8c(u8"レイヤー追加##LayerPanel_AddLayer_v10"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::AddLayer;
    }

    const bool canDelete = frame.layers.size() > 1U;
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(u8c(u8"レイヤー削除##LayerPanel_DeleteLayer_v10"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::DeleteLayer;
    }
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    if (ImGui::Button(u8c(u8"レイヤークリア##LayerPanel_ClearLayer_v10"), ImVec2(-1.0f, 0.0f))) {
        action = LayerPanelAction::ClearLayer;
    }

    if (frame.layers.empty()) {
        ImGui::TextDisabled("no layers");
        ImGui::PopID();
        return action;
    }

    activeLayerIndex = std::clamp(activeLayerIndex, 0, static_cast<int>(frame.layers.size()) - 1);
    for (int index = static_cast<int>(frame.layers.size()) - 1; index >= 0; --index) {
        Layer& layer = frame.layers[static_cast<std::size_t>(index)];
        ImGui::PushID(index);
        ImGui::Checkbox("##LayerVisible_v10", &layer.visible);
        ImGui::SameLine();
        const bool selected = activeLayerIndex == index;
        if (ImGui::Selectable(layer.name.c_str(), selected)) {
            activeLayerIndex = index;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::SliderFloat("##LayerOpacity_v10", &layer.opacity, 0.0f, 1.0f, "%.2f");
        ImGui::PopID();
    }

    ImGui::PopID();
    return action;
}

} // namespace perapera::ui
