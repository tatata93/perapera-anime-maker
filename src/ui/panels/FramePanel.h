#pragma once

// このファイルの役割:
// フレーム情報とコマ操作UIを定義する。

#include "core/Cell.h"

namespace perapera::ui {

enum class FramePanelAction {
    None,
    AddFrame,
    DuplicateFrame,
    DeleteFrame,
};

FramePanelAction drawFramePanel(Cell& cell, int& activeFrameIndex);

} // namespace perapera::ui
