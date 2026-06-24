// このファイルの役割:
// カラーパレットUIを実装する。
// 色を選ぶUIだけに責務を絞り、描画データや保存処理には直接触れない。

#include "ui/panels/ColorPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <imgui.h>

namespace perapera::ui {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

std::array<float, 4> makeColor(float r, float g, float b, float a = 1.0f)
{
    return {r, g, b, a};
}

bool sameRgb(const std::array<float, 4>& a, const std::array<float, 4>& b)
{
    const float tolerance = 0.001f;
    return std::abs(a[0] - b[0]) < tolerance &&
           std::abs(a[1] - b[1]) < tolerance &&
           std::abs(a[2] - b[2]) < tolerance;
}

void ensureDefaultPalette(ColorPanelState& state, const BrushSettings& brushSettings)
{
    if (state.initialized) {
        return;
    }

    state.editColor = brushSettings.color;
    state.swatches = {
        {u8c(u8"主線"), u8c(u8"線画"), makeColor(0.05f, 0.05f, 0.05f)},
        {u8c(u8"白"), u8c(u8"基本"), makeColor(1.0f, 1.0f, 1.0f)},
        {u8c(u8"赤トレス"), u8c(u8"色トレス"), makeColor(1.0f, 0.18f, 0.15f)},
        {u8c(u8"青トレス"), u8c(u8"色トレス"), makeColor(0.1f, 0.35f, 1.0f)},
        {u8c(u8"黄緑トレス"), u8c(u8"色トレス"), makeColor(0.55f, 1.0f, 0.15f)},
        {u8c(u8"影指定"), u8c(u8"ガイド"), makeColor(1.0f, 0.35f, 0.75f)},
    };
    state.recentColors.clear();
    state.selectedSwatchIndex = 0;
    state.initialized = true;
}

void pushRecentColor(ColorPanelState& state, const std::array<float, 4>& color)
{
    auto found = std::find_if(state.recentColors.begin(), state.recentColors.end(),
                              [&](const ColorSwatch& swatch) {
                                  return sameRgb(swatch.color, color);
                              });
    if (found != state.recentColors.end()) {
        state.recentColors.erase(found);
    }

    ColorSwatch swatch;
    swatch.name = u8c(u8"最近色");
    swatch.group = u8c(u8"履歴");
    swatch.color = color;
    state.recentColors.insert(state.recentColors.begin(), swatch);

    constexpr std::size_t maxRecentColors = 12;
    if (state.recentColors.size() > maxRecentColors) {
        state.recentColors.resize(maxRecentColors);
    }
}

void applyColorToBrush(ColorPanelState& state, BrushSettings& brushSettings, const std::array<float, 4>& color)
{
    state.editColor = color;
    brushSettings.color = color;
    pushRecentColor(state, color);
}

void drawSwatchGrid(const char* label,
                    std::vector<ColorSwatch>& swatches,
                    ColorPanelState& state,
                    BrushSettings& brushSettings,
                    bool allowDelete)
{
    if (swatches.empty()) {
        ImGui::TextDisabled(u8c(u8"色がありません。"));
        return;
    }

    ImGui::PushID(label);
    constexpr int columns = 6;
    for (int i = 0; i < static_cast<int>(swatches.size()); ++i) {
        ColorSwatch& swatch = swatches[static_cast<std::size_t>(i)];
        ImGui::PushID(i);
        const ImVec4 color(swatch.color[0], swatch.color[1], swatch.color[2], swatch.color[3]);
        if (ImGui::ColorButton("##swatch", color, ImGuiColorEditFlags_NoTooltip, ImVec2(24.0f, 24.0f))) {
            state.selectedSwatchIndex = i;
            applyColorToBrush(state, brushSettings, swatch.color);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s / %s", swatch.group.c_str(), swatch.name.c_str());
        }
        if (allowDelete && ImGui::BeginPopupContextItem("swatch_menu")) {
            if (ImGui::MenuItem(u8c(u8"削除"))) {
                swatches.erase(swatches.begin() + i);
                state.selectedSwatchIndex = -1;
                ImGui::EndPopup();
                ImGui::PopID();
                break;
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
        if ((i + 1) % columns != 0 && i + 1 < static_cast<int>(swatches.size())) {
            ImGui::SameLine();
        }
    }
    ImGui::PopID();
}

} // namespace

void drawColorPanel(ColorPanelState& state, BrushSettings& brushSettings)
{
    ensureDefaultPalette(state, brushSettings);

    ImGui::PushID("ColorPanel_v30_palette_foundation");
    ImGui::TextUnformatted(u8c(u8"カラー"));
    ImGui::Separator();

    state.editColor = brushSettings.color;
    if (ImGui::ColorEdit4(u8c(u8"現在色"), state.editColor.data(), ImGuiColorEditFlags_NoInputs)) {
        brushSettings.color = state.editColor;
    }

    if (ImGui::Button(u8c(u8"ブラシ色へ反映"), ImVec2(-1.0f, 0.0f))) {
        applyColorToBrush(state, brushSettings, state.editColor);
    }

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(u8c(u8"名前"), state.nameBuffer, sizeof(state.nameBuffer));
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText(u8c(u8"グループ"), state.groupBuffer, sizeof(state.groupBuffer));

    if (ImGui::Button(u8c(u8"現在色を追加"), ImVec2(-1.0f, 0.0f))) {
        ColorSwatch swatch;
        swatch.name = state.nameBuffer[0] == '\0' ? u8c(u8"色") : state.nameBuffer;
        swatch.group = state.groupBuffer[0] == '\0' ? u8c(u8"基本") : state.groupBuffer;
        swatch.color = brushSettings.color;
        state.swatches.push_back(swatch);
        state.selectedSwatchIndex = static_cast<int>(state.swatches.size()) - 1;
        pushRecentColor(state, brushSettings.color);
    }

    if (ImGui::TreeNodeEx(u8c(u8"スウォッチ"), ImGuiTreeNodeFlags_DefaultOpen)) {
        drawSwatchGrid("palette", state.swatches, state, brushSettings, true);
        ImGui::TextDisabled(u8c(u8"右クリックで削除。並べ替えと保存は次Stepで実装予定。"));
        ImGui::TreePop();
    }

    if (ImGui::TreeNode(u8c(u8"最近使った色"))) {
        drawSwatchGrid("recent", state.recentColors, state, brushSettings, false);
        ImGui::TreePop();
    }

    if (ImGui::Button(u8c(u8"色トレス: 赤"))) {
        applyColorToBrush(state, brushSettings, makeColor(1.0f, 0.18f, 0.15f));
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"青"))) {
        applyColorToBrush(state, brushSettings, makeColor(0.1f, 0.35f, 1.0f));
    }
    ImGui::SameLine();
    if (ImGui::Button(u8c(u8"黄緑"))) {
        applyColorToBrush(state, brushSettings, makeColor(0.55f, 1.0f, 0.15f));
    }

    ImGui::TextDisabled(u8c(u8"スポイトと palette.json 保存は後続Step。"));
    ImGui::PopID();
}

} // namespace perapera::ui
