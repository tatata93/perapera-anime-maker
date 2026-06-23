// このファイルの役割:
// Cell の初期値生成と安全なフレーム参照を実装する。

#include "core/Cell.h"

namespace perapera {

Cell Cell::createDefault()
{
    Cell cell;
    cell.id = "cell_001";
    cell.name = "キャラA";
    cell.category = "character";
    cell.widthPx = 1920;
    cell.heightPx = 1080;
    cell.zOrder = 0;
    cell.visible = true;
    cell.opacity = 1.0f;
    cell.placement = CellPlacement{};
    cell.frames.push_back(Frame::createDefault(1));
    return cell;
}

Frame* Cell::frameOrNull(int frameIndex)
{
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) {
        return nullptr;
    }
    return &frames[static_cast<std::size_t>(frameIndex)];
}

const Frame* Cell::frameOrNull(int frameIndex) const
{
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) {
        return nullptr;
    }
    return &frames[static_cast<std::size_t>(frameIndex)];
}

} // namespace perapera
