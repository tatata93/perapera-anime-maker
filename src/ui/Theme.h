#pragma once

// このファイルの役割:
// アプリ全体で使うダークテーマの色をまとめる。
// UI色を1か所に集めることで、あとからテーマ変更しやすくする。

#include <imgui.h>

namespace perapera::ui {

struct ThemeColors {
    ImVec4 darkestBackground = ImVec4(0.051f, 0.051f, 0.063f, 1.0f); // #0d0d10
    ImVec4 panelBackground   = ImVec4(0.078f, 0.078f, 0.094f, 1.0f); // #141418
    ImVec4 canvasBackground  = ImVec4(0.059f, 0.059f, 0.071f, 1.0f); // #0f0f12
    ImVec4 border            = ImVec4(0.165f, 0.165f, 0.184f, 1.0f); // #2a2a2f
    ImVec4 textMain          = ImVec4(0.800f, 0.800f, 0.800f, 1.0f); // #cccccc
    ImVec4 textSubtle        = ImVec4(0.400f, 0.400f, 0.400f, 1.0f); // #666666
    ImVec4 accent            = ImVec4(0.498f, 0.467f, 0.867f, 1.0f); // #7F77DD
};

inline const ThemeColors& themeColors()
{
    static const ThemeColors colors;
    return colors;
}

inline void applyDarkTheme()
{
    const ThemeColors& colors = themeColors();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    ImVec4* imguiColors = style.Colors;
    imguiColors[ImGuiCol_Text] = colors.textMain;
    imguiColors[ImGuiCol_TextDisabled] = colors.textSubtle;
    imguiColors[ImGuiCol_WindowBg] = colors.darkestBackground;
    imguiColors[ImGuiCol_ChildBg] = colors.panelBackground;
    imguiColors[ImGuiCol_PopupBg] = colors.panelBackground;
    imguiColors[ImGuiCol_Border] = colors.border;
    imguiColors[ImGuiCol_FrameBg] = ImVec4(0.105f, 0.105f, 0.125f, 1.0f);
    imguiColors[ImGuiCol_FrameBgHovered] = ImVec4(0.145f, 0.145f, 0.180f, 1.0f);
    imguiColors[ImGuiCol_FrameBgActive] = ImVec4(0.200f, 0.190f, 0.320f, 1.0f);
    imguiColors[ImGuiCol_TitleBg] = colors.panelBackground;
    imguiColors[ImGuiCol_TitleBgActive] = colors.panelBackground;
    imguiColors[ImGuiCol_MenuBarBg] = colors.panelBackground;
    imguiColors[ImGuiCol_ScrollbarBg] = colors.darkestBackground;
    imguiColors[ImGuiCol_ScrollbarGrab] = ImVec4(0.180f, 0.180f, 0.220f, 1.0f);
    imguiColors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.240f, 0.240f, 0.300f, 1.0f);
    imguiColors[ImGuiCol_ScrollbarGrabActive] = colors.accent;
    imguiColors[ImGuiCol_CheckMark] = colors.accent;
    imguiColors[ImGuiCol_SliderGrab] = colors.accent;
    imguiColors[ImGuiCol_SliderGrabActive] = ImVec4(0.620f, 0.590f, 1.0f, 1.0f);
    imguiColors[ImGuiCol_Button] = ImVec4(0.130f, 0.130f, 0.160f, 1.0f);
    imguiColors[ImGuiCol_ButtonHovered] = ImVec4(0.200f, 0.190f, 0.320f, 1.0f);
    imguiColors[ImGuiCol_ButtonActive] = colors.accent;
    imguiColors[ImGuiCol_Header] = ImVec4(0.160f, 0.150f, 0.260f, 1.0f);
    imguiColors[ImGuiCol_HeaderHovered] = ImVec4(0.220f, 0.210f, 0.360f, 1.0f);
    imguiColors[ImGuiCol_HeaderActive] = colors.accent;
    imguiColors[ImGuiCol_Tab] = colors.panelBackground;
    imguiColors[ImGuiCol_TabHovered] = ImVec4(0.220f, 0.210f, 0.360f, 1.0f);
    imguiColors[ImGuiCol_TabActive] = colors.accent;
    imguiColors[ImGuiCol_TabUnfocused] = colors.panelBackground;
    imguiColors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.200f, 0.190f, 0.320f, 1.0f);
}

} // namespace perapera::ui
