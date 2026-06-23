#pragma once

// このファイルの役割:
// 作画モード下部の簡易タイムラインUIを定義する。
// 右パネルだけでなくタイムライン側からもフレーム操作できるようにする。

#include "core/Cell.h"

namespace perapera::ui {

enum class TimelinePanelAction {
    None,
    AddFrame,
    DuplicateFrame,
    DeleteFrame,
};

TimelinePanelAction drawTimelinePanel(Cell& cell,
                                      int& activeFrameIndex,
                                      bool& onionPrevious,
                                      bool& onionNext);

} // namespace perapera::ui
