// src/ui/panels/ProjectPanel.h
//
// プロジェクト保存/読み込みのUI状態を担当します。

#pragma once

#include <string>

namespace perapera
{
    enum class ProjectPanelAction
    {
        None,
        Save,
        Load
    };

    class ProjectPanel
    {
    public:
        ProjectPanelAction draw();

        const std::string& projectFilePath() const;

        void setResult(
            bool succeeded,
            const std::string& message
        );

    private:
        std::string projectFilePath_ =
            "projects/current_project.perapera_project.txt";

        std::string resultMessage_;
        bool resultSucceeded_ = false;
    };
}
