#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "core/TimesheetSceneResolver.h"
#include "io/PngExporter.h"

namespace perapera {

class PngTimesheetExporter {
public:
    static bool exportFrame(const Timesheet& timesheet,
                            const std::vector<TimesheetSceneCellInput>& inputs,
                            int timelineFrame,
                            const std::filesystem::path& outputPath,
                            int width,
                            int height,
                            ExportMode mode = ExportMode::Composite,
                            std::string* errorMessage = nullptr);

    static bool exportFrameSequence(const Timesheet& timesheet,
                                    const std::vector<TimesheetSceneCellInput>& inputs,
                                    const std::filesystem::path& outputFolder,
                                    int width,
                                    int height,
                                    ExportMode mode = ExportMode::Composite,
                                    std::string* errorMessage = nullptr);
};

} // namespace perapera
