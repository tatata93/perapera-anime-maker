// このファイルの役割:
// レイヤー・フレーム操作など、Projectを変更する小さな操作を実装する。
// フレーム操作後は名前を振り直し、タイムライン表示と保存フォルダ名が分かりやすくなるようにする。

#include "ui/App.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>

namespace perapera {
namespace {

std::string frameNameForIndex(int zeroBasedIndex)
{
    std::ostringstream name;
    name << "frame_" << std::setw(3) << std::setfill('0') << (zeroBasedIndex + 1);
    return name.str();
}

void renumberFrames(Cell& cell)
{
    for (int index = 0; index < static_cast<int>(cell.frames.size()); ++index) {
        cell.frames[static_cast<std::size_t>(index)].name = frameNameForIndex(index);
    }
}

} // namespace

void App::addLayer()
{
    Frame* frame = activeFrame();
    if (frame == nullptr) {
        return;
    }

    pushUndoSnapshot();
    const int newNumber = static_cast<int>(frame->layers.size()) + 1;
    frame->layers.push_back(Layer::createDefault(newNumber));
    activeLayerIndex_ = static_cast<int>(frame->layers.size()) - 1;
    canvasRenderer_.markAllDirty();
    lastMessage_ = "layer added";
}

void App::deleteActiveLayer()
{
    Frame* frame = activeFrame();
    if (frame == nullptr || frame->layers.size() <= 1U) {
        lastMessage_ = "cannot delete last layer";
        return;
    }

    pushUndoSnapshot();
    activeLayerIndex_ = std::clamp(activeLayerIndex_, 0, static_cast<int>(frame->layers.size()) - 1);
    frame->layers.erase(frame->layers.begin() + activeLayerIndex_);
    clampSelection();
    canvasRenderer_.markAllDirty();
    lastMessage_ = "layer deleted";
}

void App::clearActiveLayer()
{
    Layer* layer = activeLayer();
    if (layer == nullptr) {
        return;
    }

    pushUndoSnapshot();
    layer->strokes.clear();
    canvasRenderer_.clearLayer(activeLayerIndex_);
    canvasRenderer_.markDirty(activeLayerIndex_);
    lastMessage_ = "layer cleared";
}

void App::addFrame()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        lastMessage_ = "frame add failed: no active cell";
        return;
    }

    pushUndoSnapshot();

    const int frameCount = static_cast<int>(cell->frames.size());
    const int insertIndex = frameCount <= 0
        ? 0
        : std::clamp(activeFrameIndex_ + 1, 0, frameCount);

    cell->frames.insert(cell->frames.begin() + insertIndex, Frame::createDefault(insertIndex + 1));
    renumberFrames(*cell);
    activeFrameIndex_ = insertIndex;
    activeLayerIndex_ = 0;
    clampSelection();
    canvasRenderer_.markAllDirty();
    lastMessage_ = "frame added: count=" + std::to_string(cell->frames.size());
}

void App::duplicateFrame()
{
    Cell* cell = activeCell();
    Frame* frame = activeFrame();
    if (cell == nullptr || frame == nullptr) {
        lastMessage_ = "frame duplicate failed: no active frame";
        return;
    }

    pushUndoSnapshot();
    const int insertIndex = std::clamp(activeFrameIndex_ + 1, 0, static_cast<int>(cell->frames.size()));
    Frame duplicate = *frame;
    cell->frames.insert(cell->frames.begin() + insertIndex, duplicate);
    renumberFrames(*cell);
    activeFrameIndex_ = insertIndex;
    activeLayerIndex_ = 0;
    clampSelection();
    canvasRenderer_.markAllDirty();
    lastMessage_ = "frame duplicated: count=" + std::to_string(cell->frames.size());
}

void App::deleteActiveFrame()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        lastMessage_ = "frame delete failed: no active cell";
        return;
    }
    if (cell->frames.size() <= 1U) {
        lastMessage_ = "cannot delete last frame";
        return;
    }

    pushUndoSnapshot();
    activeFrameIndex_ = std::clamp(activeFrameIndex_, 0, static_cast<int>(cell->frames.size()) - 1);
    cell->frames.erase(cell->frames.begin() + activeFrameIndex_);
    renumberFrames(*cell);
    activeFrameIndex_ = std::clamp(activeFrameIndex_, 0, static_cast<int>(cell->frames.size()) - 1);
    activeLayerIndex_ = 0;
    clampSelection();
    canvasRenderer_.markAllDirty();
    lastMessage_ = "frame deleted: count=" + std::to_string(cell->frames.size());
}

} // namespace perapera
