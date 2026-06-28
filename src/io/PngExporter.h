#pragma once

// このファイルの役割:
// Frame/Cellのストローク点列をCPU上でラスタライズし、PNG画像として書き出す入口を定義する。

#include <filesystem>
#include <string>
#include <vector>

#include "core/Cell.h"
#include "core/Frame.h"

namespace perapera {

// Phase 1.5 Step 15: PNG/MP4書き出し時のレイヤー選別モード。
enum class ExportMode {
    Composite,
    LineTest,
    ColorTrace,
    LineOnly,
};

struct ExportCellFrameRef {
    const Cell* cell = nullptr;
    const Frame* frame = nullptr;
};

const char* exportModeToString(ExportMode mode);
const char* exportModeDisplayName(ExportMode mode);

class PngExporter {
public:
    static bool exportFrame(const Frame& frame,
                            const std::filesystem::path& outputPath,
                            int width,
                            int height,
                            ExportMode mode = ExportMode::Composite,
                            std::string* errorMessage = nullptr);

    static bool exportFrameSequence(const Cell& cell,
                                    const std::filesystem::path& outputFolder,
                                    int width,
                                    int height,
                                    ExportMode mode = ExportMode::Composite,
                                    std::string* errorMessage = nullptr);

    static bool exportCellsFrame(const std::vector<const Cell*>& cells,
                                 int frameIndex,
                                 const std::filesystem::path& outputPath,
                                 int width,
                                 int height,
                                 ExportMode mode = ExportMode::Composite,
                                 std::string* errorMessage = nullptr);

    static bool exportCellFrameRefs(const std::vector<ExportCellFrameRef>& refs,
                                    const std::filesystem::path& outputPath,
                                    int width,
                                    int height,
                                    ExportMode mode = ExportMode::Composite,
                                    std::string* errorMessage = nullptr);

    static bool exportCellsFrameSequence(const std::vector<const Cell*>& cells,
                                         const std::filesystem::path& outputFolder,
                                         int width,
                                         int height,
                                         ExportMode mode = ExportMode::Composite,
                                         std::string* errorMessage = nullptr);

    static bool exportCellFrameRefSequence(const std::vector<std::vector<ExportCellFrameRef>>& frames,
                                           const std::filesystem::path& outputFolder,
                                           int width,
                                           int height,
                                           ExportMode mode = ExportMode::Composite,
                                           std::string* errorMessage = nullptr);
};

} // namespace perapera
