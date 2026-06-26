// このファイルの役割:
// 作画キャンバス内のマウス入力を処理する。
// 入力中ストロークだけDrawListでプレビューし、確定時にProjectとCanvasBitmapへ反映する。
// Phase 1.5 Step 20: フレーム移動では全dirtyを出さず、FloodFill確定は対象レイヤーだけdirty化する。

#include "ui/App.h"

#include <algorithm>
#include <cmath>

#include <imgui.h>

#include "fill/FloodFill.h"

namespace perapera {
namespace {

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

float distanceSquared(const StrokePoint& a, const StrokePoint& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

bool isPointInside(ImVec2 point, ImVec2 min, ImVec2 size)
{
    return point.x >= min.x && point.y >= min.y &&
           point.x <= min.x + size.x && point.y <= min.y + size.y;
}

} // namespace

bool App::stepActiveFrame(int delta)
{
    Cell* cell = activeCell();
    if (cell == nullptr || cell->frames.empty()) {
        return false;
    }

    const int previousIndex = activeFrameIndex_;
    const int lastIndex = static_cast<int>(cell->frames.size()) - 1;
    activeFrameIndex_ = std::clamp(activeFrameIndex_ + delta, 0, lastIndex);

    if (activeFrameIndex_ == previousIndex) {
        return false;
    }

    clampSelection();
    // フレーム移動は表示対象キーが変わるだけなので、全dirtyは不要。
    // ここでmarkAllDirty()すると任意フレーム選択のたびにキャッシュ判定・再ベイク待ちが出る。
    lastMessage_ = "frame stepped to " + std::to_string(activeFrameIndex_ + 1);
    return true;
}

bool App::advancePlaybackFrame()
{
    Cell* cell = activeCell();
    if (cell == nullptr || cell->frames.size() <= 1U) {
        isPlayingFrames_ = false;
        return false;
    }

    const int lastIndex = static_cast<int>(cell->frames.size()) - 1;
    int nextIndex = activeFrameIndex_ + playbackDirection_;

    if (nextIndex < 0 || nextIndex > lastIndex) {
        if (playbackPingPong_) {
            playbackDirection_ = -playbackDirection_;
            nextIndex = activeFrameIndex_ + playbackDirection_;
        } else {
            nextIndex = nextIndex > lastIndex ? 0 : lastIndex;
        }
    }

    nextIndex = std::clamp(nextIndex, 0, lastIndex);
    if (nextIndex == activeFrameIndex_) {
        return false;
    }

    activeFrameIndex_ = nextIndex;
    clampSelection();
    return true;
}

void App::updateFramePlayback()
{
    if (!isPlayingFrames_) {
        playbackAccumulator_ = 0.0f;
        return;
    }
    if (isDrawingStroke_) {
        return;
    }

    Cell* cell = activeCell();
    if (cell == nullptr || cell->frames.size() <= 1U) {
        isPlayingFrames_ = false;
        playbackAccumulator_ = 0.0f;
        return;
    }

    const Frame* frame = activeFrame();
    const int durationFrames = frame == nullptr ? 1 : std::max(1, frame->durationFrames);
    const float baseFps = 24.0f;
    const float secondsPerStep = std::max(1.0f / 60.0f, static_cast<float>(durationFrames) / (baseFps * playbackSpeed_));
    playbackAccumulator_ += std::clamp(ImGui::GetIO().DeltaTime, 0.0f, 0.1f);

    while (playbackAccumulator_ >= secondsPerStep) {
        playbackAccumulator_ -= secondsPerStep;
        if (!advancePlaybackFrame()) {
            break;
        }
    }
}

void App::warmPlaybackFrameCache()
{
    if (!isPlayingFrames_ || isDrawingStroke_) {
        return;
    }

    const Cell* cell = activeCell();
    if (cell == nullptr || cell->frames.size() <= 1U) {
        return;
    }

    const int lastIndex = static_cast<int>(cell->frames.size()) - 1;
    int direction = playbackDirection_ == 0 ? 1 : playbackDirection_;
    int cursor = activeFrameIndex_;

    constexpr int kLookAheadFrames = 3;
    constexpr int kWarmLayerBudget = 1;
    constexpr int kWarmStrokeBudgetPerLayer = 1;

    const CanvasDisplayMode displayMode = currentMode_ == AppMode::Coloring
        ? CanvasDisplayMode::Coloring
        : CanvasDisplayMode::Drawing;

    for (int step = 0; step < kLookAheadFrames; ++step) {
        int nextIndex = cursor + direction;
        if (nextIndex < 0 || nextIndex > lastIndex) {
            if (playbackPingPong_) {
                direction = -direction;
                nextIndex = cursor + direction;
            } else {
                nextIndex = nextIndex > lastIndex ? 0 : lastIndex;
            }
        }

        nextIndex = std::clamp(nextIndex, 0, lastIndex);
        if (nextIndex == activeFrameIndex_) {
            break;
        }

        const Frame* frame = cell->frameOrNull(nextIndex);
        if (frame != nullptr) {
            canvasRenderer_.warmFrameCache(*frame,
                                           nextIndex,
                                           displayMode,
                                           kWarmLayerBudget,
                                           kWarmStrokeBudgetPerLayer);
        }
        cursor = nextIndex;
    }
}

void App::handleFrameShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || isDrawingStroke_) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_B)) {
        brushSettings_.tool = ui::ToolKind::Brush;
        lastMessage_ = "tool: brush";
    }
    if (ImGui::IsKeyPressed(ImGuiKey_E)) {
        brushSettings_.tool = ui::ToolKind::Eraser;
        lastMessage_ = "tool: eraser";
    }
    if (ImGui::IsKeyPressed(ImGuiKey_G)) {
        brushSettings_.tool = ui::ToolKind::FloodFill;
        lastMessage_ = "tool: flood fill";
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        isPlayingFrames_ = !isPlayingFrames_;
        playbackAccumulator_ = 0.0f;
        lastMessage_ = isPlayingFrames_ ? "finger preview playing" : "finger preview stopped";
    }

    if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        isPlayingFrames_ = false;
        playbackAccumulator_ = 0.0f;
        stepActiveFrame(-1);
    }
    if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        isPlayingFrames_ = false;
        playbackAccumulator_ = 0.0f;
        stepActiveFrame(1);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        Cell* cell = activeCell();
        if (cell != nullptr && !cell->frames.empty() && activeFrameIndex_ != 0) {
            isPlayingFrames_ = false;
            activeFrameIndex_ = 0;
            clampSelection();
            lastMessage_ = "frame stepped to 1";
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        Cell* cell = activeCell();
        if (cell != nullptr && !cell->frames.empty()) {
            const int lastIndex = static_cast<int>(cell->frames.size()) - 1;
            if (activeFrameIndex_ != lastIndex) {
                isPlayingFrames_ = false;
                activeFrameIndex_ = lastIndex;
                clampSelection();
                lastMessage_ = "frame stepped to " + std::to_string(activeFrameIndex_ + 1);
            }
        }
    }
}

void App::drawFingerPlaybackControls()
{
    Cell* cell = activeCell();
    const int frameCount = cell == nullptr ? 0 : static_cast<int>(cell->frames.size());

    ImGui::PushID("FingerPlayback_v28_controls");
    ImGui::TextUnformatted(u8c(u8"指パラ"));
    ImGui::SameLine();
    if (ImGui::Button("|<")) {
        if (cell != nullptr && !cell->frames.empty()) {
            isPlayingFrames_ = false;
            activeFrameIndex_ = 0;
            clampSelection();
            lastMessage_ = "frame stepped to 1";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("<")) {
        isPlayingFrames_ = false;
        stepActiveFrame(-1);
    }
    ImGui::SameLine();
    if (ImGui::Button(isPlayingFrames_ ? u8c(u8"停止 Space") : u8c(u8"再生 Space"))) {
        isPlayingFrames_ = !isPlayingFrames_;
        playbackAccumulator_ = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button(">")) {
        isPlayingFrames_ = false;
        stepActiveFrame(1);
    }
    ImGui::SameLine();
    if (ImGui::Button(">|")) {
        if (cell != nullptr && !cell->frames.empty()) {
            isPlayingFrames_ = false;
            activeFrameIndex_ = static_cast<int>(cell->frames.size()) - 1;
            clampSelection();
            lastMessage_ = "frame stepped to " + std::to_string(activeFrameIndex_ + 1);
        }
    }

    const char* speedItems[] = {"0.25x", "0.5x", "1x", "2x"};
    int speedIndex = 2;
    if (playbackSpeed_ <= 0.26f) {
        speedIndex = 0;
    } else if (playbackSpeed_ <= 0.51f) {
        speedIndex = 1;
    } else if (playbackSpeed_ >= 1.99f) {
        speedIndex = 3;
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    if (ImGui::Combo(u8c(u8"速度"), &speedIndex, speedItems, IM_ARRAYSIZE(speedItems))) {
        const float speeds[] = {0.25f, 0.5f, 1.0f, 2.0f};
        playbackSpeed_ = speeds[std::clamp(speedIndex, 0, 3)];
        playbackAccumulator_ = 0.0f;
    }
    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"ピンポン"), &playbackPingPong_);
    ImGui::SameLine();
    ImGui::Checkbox(u8c(u8"再生中は補助非表示"), &playbackSkipAssistOverlays_);
    ImGui::SameLine();
    ImGui::Text("%d/%d", frameCount > 0 ? activeFrameIndex_ + 1 : 0, frameCount);
    ImGui::TextDisabled(u8c(u8"Shift+←/→: 前後   Home/End: 先頭/末尾"));
    ImGui::PopID();
}

void App::handleCanvasInput(ImVec2 areaMin, ImVec2 areaSize)
{
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mouse = io.MousePos;
    const bool hovered = ImGui::IsWindowHovered() && isPointInside(mouse, areaMin, areaSize);

    if (hovered && io.MouseWheel != 0.0f) {
        const ImVec2 before = canvasView_.screenToCanvas(mouse.x, mouse.y, areaMin, areaSize);
        const float zoomFactor = io.MouseWheel > 0.0f ? 1.15f : 1.0f / 1.15f;
        canvasView_.zoom = std::clamp(canvasView_.zoom * zoomFactor, 0.05f, 32.0f);
        const ImVec2 after = canvasView_.screenToCanvas(mouse.x, mouse.y, areaMin, areaSize);
        canvasView_.panX += (after.x - before.x) * canvasView_.zoom;
        canvasView_.panY += (after.y - before.y) * canvasView_.zoom;
    }

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        canvasView_.panX += io.MouseDelta.x;
        canvasView_.panY += io.MouseDelta.y;
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        beginStroke(mouse, areaMin, areaSize);
    }

    if (isDrawingStroke_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        updateStroke(mouse, areaMin, areaSize);
    }

    if (isDrawingStroke_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        finishStroke();
    }

    if (isDrawingStroke_ && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        cancelStroke();
    }
}

void App::applyFloodFillAt(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize)
{
    Frame* frame = activeFrame();
    Layer* layer = activeLayer();
    if (frame == nullptr || layer == nullptr) {
        return;
    }
    if (layer->type != LayerType::Paint) {
        lastMessage_ = "flood fill requires active Paint layer";
        return;
    }

    const ImVec2 canvas = canvasView_.screenToCanvas(mouseScreen.x, mouseScreen.y, areaMin, areaSize);
    fill::FloodFillSettings settings;
    settings.tolerance = brushSettings_.fillTolerance;
    settings.gapClosePx = brushSettings_.fillGapClosePx;
    settings.insetPx = brushSettings_.fillInsetPx;
    settings.leakGuardPercent = brushSettings_.fillLeakGuardPercent;

    const fill::FloodFillResult result = fill::makeFloodFillStrokes(*frame,
                                                                    activeLayerIndex_,
                                                                    project_.canvas.width,
                                                                    project_.canvas.height,
                                                                    static_cast<int>(std::lround(canvas.x)),
                                                                    static_cast<int>(std::lround(canvas.y)),
                                                                    brushSettings_.color,
                                                                    settings);
    if (!result.success || result.strokes.empty()) {
        lastMessage_ = "flood fill: " + result.message;
        return;
    }

    pushUndoSnapshot();
    Layer* targetLayer = activeLayer();
    if (targetLayer == nullptr || targetLayer->type != LayerType::Paint) {
        return;
    }
    targetLayer->strokes.insert(targetLayer->strokes.end(), result.strokes.begin(), result.strokes.end());
    targetLayer->touchRevision();
    canvasRenderer_.markDirty(activeLayerIndex_);
    lastMessage_ = "flood fill: " + std::to_string(result.filledPixelCount) +
                   " px / " + std::to_string(result.strokes.size()) + " fills";
}

void App::beginStroke(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize)
{
    if (currentMode_ == AppMode::Coloring) {
        selectPaintLayerForColoring(true);
    }

    Layer* layer = activeLayer();
    if (layer == nullptr) {
        return;
    }

    if (currentMode_ == AppMode::Coloring && layer->type != LayerType::Paint) {
        lastMessage_ = "coloring mode edits Paint layer only";
        return;
    }

    if (brushSettings_.tool == ui::ToolKind::FloodFill) {
        applyFloodFillAt(mouseScreen, areaMin, areaSize);
        return;
    }

    currentStroke_ = Stroke{};
    isMyPaintStrokeActive_ = false;
    currentStroke_.color = brushSettings_.color;
    currentStroke_.radiusPx = brushSettings_.radiusPx;
    currentStroke_.brushEngine = brushSettings_.engine == ui::BrushEngineKind::MyPaint
        ? StrokeBrushEngine::MyPaint
        : StrokeBrushEngine::Simple;
    currentStroke_.opacity = std::clamp(brushSettings_.opacity, 0.05f, 1.0f);
    currentStroke_.hardness = std::clamp(brushSettings_.hardness, 0.0f, 1.0f);
    currentStroke_.spacing = std::clamp(brushSettings_.spacing, 0.05f, 1.0f);
    currentStroke_.pressureToSize = std::clamp(brushSettings_.pressureToSize, 0.0f, 1.0f);
    currentStroke_.pressureToOpacity = std::clamp(brushSettings_.pressureToOpacity, 0.0f, 1.0f);
    currentStroke_.watercolorBleed = std::clamp(brushSettings_.watercolorBleed, 0.0f, 1.0f);
    currentStroke_.colorMix = std::clamp(brushSettings_.colorMix, 0.0f, 1.0f);
    currentStroke_.dryRate = std::clamp(brushSettings_.dryRate, 0.0f, 1.0f);

    if (brushSettings_.tool == ui::ToolKind::Eraser) {
        currentStroke_.color = {1.0f, 0.2f, 0.2f, 0.75f};
    }

    isMyPaintStrokeActive_ = false;
    if (brushSettings_.tool == ui::ToolKind::Brush &&
        currentStroke_.brushEngine == StrokeBrushEngine::MyPaint &&
        myPaintEngine_.isLibraryAvailable()) {
        CanvasBitmap* bitmap = canvasRenderer_.bitmapForLayerPtr(activeLayerIndex_);
        if (bitmap != nullptr) {
            myPaintEngine_.beginStroke(*bitmap, currentStroke_, currentStroke_.opacity);
            isMyPaintStrokeActive_ = true;
        }
    }

    isDrawingStroke_ = true;
    updateStroke(mouseScreen, areaMin, areaSize);
}

void App::updateStroke(ImVec2 mouseScreen, ImVec2 areaMin, ImVec2 areaSize)
{
    if (!isDrawingStroke_) {
        return;
    }

    const ImVec2 canvas = canvasView_.screenToCanvas(mouseScreen.x, mouseScreen.y, areaMin, areaSize);
    StrokePoint point;
    point.x = std::clamp(canvas.x, 0.0f, static_cast<float>(std::max(1, project_.canvas.width)));
    point.y = std::clamp(canvas.y, 0.0f, static_cast<float>(std::max(1, project_.canvas.height)));
    point.pressure = 1.0f;

    const bool useMyPaintRealtime = isMyPaintStrokeActive_;
    if (currentStroke_.points.empty()) {
        currentStroke_.points.push_back(point);
        if (useMyPaintRealtime) {
            myPaintEngine_.addPoint(point, std::max(0.006f, ImGui::GetIO().DeltaTime));
        }
        return;
    }

    StrokePoint filteredPoint = point;
    if (brushSettings_.tool == ui::ToolKind::Brush && brushSettings_.stabilizer > 0.0f) {
        const StrokePoint& previous = currentStroke_.points.back();
        const float follow = 1.0f - std::clamp(brushSettings_.stabilizer, 0.0f, 1.0f) * 0.85f;
        filteredPoint.x = previous.x + (point.x - previous.x) * follow;
        filteredPoint.y = previous.y + (point.y - previous.y) * follow;
    }

    const float minDistance = std::max(0.5f, currentStroke_.radiusPx * currentStroke_.spacing);
    if (distanceSquared(currentStroke_.points.back(), filteredPoint) >= minDistance * minDistance) {
        currentStroke_.points.push_back(filteredPoint);
        if (useMyPaintRealtime) {
            myPaintEngine_.addPoint(filteredPoint, std::max(0.006f, ImGui::GetIO().DeltaTime));
        }
    }
}

} // namespace perapera
