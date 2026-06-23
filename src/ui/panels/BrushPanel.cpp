// このファイルの役割:
// ブラシ設定パネルの実装。

#include "ui/panels/BrushPanel.h"

#include <algorithm>

#include <imgui.h>

namespace perapera::ui {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

} // namespace

void drawBrushPanel(BrushSettings& settings)
{
    ImGui::TextUnformatted(u8c(u8"ブラシ"));
    ImGui::Separator();

    int tool = settings.tool == ToolKind::Brush ? 0 : 1;
    if (ImGui::RadioButton(u8c(u8"ブラシ"), tool == 0)) {
        tool = 0;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton(u8c(u8"消しゴム"), tool == 1)) {
        tool = 1;
    }
    settings.tool = tool == 0 ? ToolKind::Brush : ToolKind::Eraser;

    ImGui::SliderFloat(u8c(u8"サイズ"), &settings.radiusPx, 1.0f, 64.0f, "%.1f px");
    ImGui::SliderFloat(u8c(u8"不透明度"), &settings.opacity, 0.05f, 1.0f, "%.2f");
    settings.opacity = std::clamp(settings.opacity, 0.05f, 1.0f);

    ImGui::ColorEdit4(u8c(u8"色"), settings.color.data(), ImGuiColorEditFlags_NoInputs);
}

} // namespace perapera::ui
