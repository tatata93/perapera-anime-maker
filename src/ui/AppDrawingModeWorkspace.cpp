#include "ui/AppDrawingModeWorkspace.h"

#include "imgui.h"

#include <algorithm>

namespace perapera::app_drawing {

void drawDrawingWorkspaceLayout(const DrawingWorkspaceLayoutCallbacks& callbacks)
{
    ImGui::BeginChild(
        "DrawingWorkspace",
        ImVec2(0.0f, -28.0f),
        true,
        ImGuiWindowFlags_NoScrollWithMouse);

    const float sideWidth = 245.0f;
    const float rightWidth = 315.0f;
    const float timelineHeight = 215.0f;
    const float centerHeight = std::max(
        160.0f,
        ImGui::GetContentRegionAvail().y - timelineHeight - ImGui::GetStyle().ItemSpacing.y);

    ImGui::BeginChild(
        "DrawingUpperArea",
        ImVec2(0.0f, centerHeight),
        false,
        ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::BeginChild("DrawingLeftSidebar", ImVec2(sideWidth, 0.0f), true);
    if (callbacks.drawLeftSidebar) {
        callbacks.drawLeftSidebar();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild(
        "DrawingCanvasHost",
        ImVec2(-(rightWidth + ImGui::GetStyle().ItemSpacing.x), 0.0f),
        true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    if (callbacks.drawCanvasArea) {
        callbacks.drawCanvasArea(rightWidth);
    }
    ImGui::SetScrollX(0.0f);
    ImGui::SetScrollY(0.0f);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild(
        "DrawingRightSidebar_v5",
        ImVec2(rightWidth, 0.0f),
        true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushID("DrawingRightSidebar_v5Content_v4");
    if (callbacks.drawRightSidebar) {
        callbacks.drawRightSidebar();
    }
    ImGui::PopID();
    ImGui::EndChild();

    ImGui::EndChild();

    if (callbacks.drawTimelineArea) {
        callbacks.drawTimelineArea();
    }

    ImGui::EndChild();
}

} // namespace perapera::app_drawing