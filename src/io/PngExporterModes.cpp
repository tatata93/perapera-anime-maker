#include "io/PngExporter.h"

namespace perapera {

const char* exportModeToString(ExportMode mode)
{
    switch (mode) {
    case ExportMode::Composite:
        return "Composite";
    case ExportMode::LineTest:
        return "LineTest";
    case ExportMode::ColorTrace:
        return "ColorTrace";
    case ExportMode::LineOnly:
        return "LineOnly";
    }
    return "Composite";
}

const char* exportModeDisplayName(ExportMode mode)
{
    switch (mode) {
    case ExportMode::Composite:
        return "通常（全レイヤー合成）";
    case ExportMode::LineTest:
        return "ラインテスト（線画のみ・白背景）";
    case ExportMode::ColorTrace:
        return "色トレスアニメ（線画＋色トレス線・白背景）";
    case ExportMode::LineOnly:
        return "線画素材（線画のみ・背景透明）";
    }
    return "通常（全レイヤー合成）";
}

} // namespace perapera
