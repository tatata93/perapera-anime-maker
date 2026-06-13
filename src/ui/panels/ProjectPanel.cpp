// src/ui/panels/ProjectPanel.cpp

#include "ui/panels/ProjectPanel.h"

#include "imgui.h"

namespace perapera
{
    ProjectPanelAction ProjectPanel::draw()
    {
        ProjectPanelAction action = ProjectPanelAction::None;

        ImGui::Begin("プロジェクト");

        ImGui::Text(
            "保存先: %s",
            projectFilePath_.c_str()
        );
        ImGui::Text(
            "読み込み時は、現在の作業内容をファイル内容で置き換えます。"
        );

        if (ImGui::Button("プロジェクト保存"))
        {
            action = ProjectPanelAction::Save;
        }

        ImGui::SameLine();

        if (ImGui::Button("プロジェクト読み込み"))
        {
            action = ProjectPanelAction::Load;
        }

        if (!resultMessage_.empty())
        {
            ImGui::TextColored(
                resultSucceeded_
                    ? ImVec4(0.45f, 1.0f, 0.55f, 1.0f)
                    : ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                "%s",
                resultMessage_.c_str()
            );
        }

        ImGui::End();
        return action;
    }

    const std::string& ProjectPanel::projectFilePath() const
    {
        return projectFilePath_;
    }

    void ProjectPanel::setResult(
        bool succeeded,
        const std::string& message
    )
    {
        resultSucceeded_ = succeeded;
        resultMessage_ = message;
    }
}
