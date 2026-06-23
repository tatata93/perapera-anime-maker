#pragma once

// このファイルの役割:
// 作画モード右サイドバーのレイヤー操作UIを定義する。

#include "core/Frame.h"

namespace perapera::ui {

enum class LayerPanelAction {
    None,
    AddLayer,
    DeleteLayer,
    ClearLayer,
};

LayerPanelAction drawLayerPanel(Frame& frame, int& activeLayerIndex);

} // namespace perapera::ui
