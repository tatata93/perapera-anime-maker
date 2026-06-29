// このファイルの役割:
#include "ui/App.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <imgui.h>
#include "core/TimesheetResolver.h"
#include "core/TimesheetInbetweenResolver.h"
#include "core/TimesheetSceneResolver.h"
#include "io/TimesheetIO.h"
#include "ui/AppProjectIOSupport.h"
#include "ui/Theme.h"
#include "ui/panels/BrushPanel.h"
#include "ui/panels/CellPanel.h"
#include "ui/panels/ColorPanel.h"
#include "ui/panels/ExportPanel.h"
#include "ui/panels/FramePanel.h"
#include "ui/panels/LayerPanel.h"
#include "ui/panels/TimelinePanel.h"
#include "ui/panels/TimesheetPanel.h"
#include "ui/panels/TimesheetPanelBridge.h"
namespace perapera {
namespace {
const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

int countTimesheetEntries(const Timesheet& timesheet)
{
    int count = 0;
    for (const TimesheetCellTrack& track : timesheet.tracks) {
        count += static_cast<int>(track.entries.size());
    }
    return count;
}
float distanceSquared(const StrokePoint& a, const StrokePoint& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}
float distancePointToSegmentSquared(const StrokePoint& point,
                                    const StrokePoint& segmentA,
                                    const StrokePoint& segmentB)
{
    const float vx = segmentB.x - segmentA.x;
    const float vy = segmentB.y - segmentA.y;
    const float wx = point.x - segmentA.x;
    const float wy = point.y - segmentA.y;
    const float lengthSq = vx * vx + vy * vy;
    if (lengthSq <= 0.0001f) {
        return distanceSquared(point, segmentA);
    }
    const float t = std::clamp((wx * vx + wy * vy) / lengthSq, 0.0f, 1.0f);
    const StrokePoint closest{segmentA.x + vx * t, segmentA.y + vy * t, 1.0f};
    return distanceSquared(point, closest);
}
float distancePointToStrokePathSquared(const StrokePoint& point, const Stroke& stroke)
{
    if (stroke.points.empty()) {
        return std::numeric_limits<float>::infinity();
    }
    if (stroke.points.size() == 1U) {
        return distanceSquared(point, stroke.points.front());
    }
    float best = std::numeric_limits<float>::infinity();
    for (std::size_t i = 1; i < stroke.points.size(); ++i) {
        best = std::min(best,
                        distancePointToSegmentSquared(point,
                                                       stroke.points[i - 1U],
                                                       stroke.points[i]));
    }
    return best;
}
float strokeApproxLength(const Stroke& stroke)
{
    if (stroke.points.size() < 2U) {
        return 0.0f;
    }
    float length = 0.0f;
    for (std::size_t i = 1; i < stroke.points.size(); ++i) {
        const float dx = stroke.points[i].x - stroke.points[i - 1U].x;
        const float dy = stroke.points[i].y - stroke.points[i - 1U].y;
        length += std::sqrt(dx * dx + dy * dy);
    }
    return length;
}
std::vector<StrokePoint> resampleStrokeForEraser(const Stroke& stroke, float spacing)
{
    std::vector<StrokePoint> result;
    if (stroke.points.empty()) {
        return result;
    }
    if (stroke.points.size() == 1U) {
        result.push_back(stroke.points.front());
        return result;
    }
    result.push_back(stroke.points.front());
    const float safeSpacing = std::max(0.5f, spacing);
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const float dx = current.x - previous.x;
        const float dy = current.y - previous.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        const int steps = std::max(1, static_cast<int>(std::ceil(length / safeSpacing)));
        for (int step = 1; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            result.push_back(StrokePoint{previous.x + dx * t,
                                         previous.y + dy * t,
                                         previous.pressure + (current.pressure - previous.pressure) * t});
        }
    }
    return result;
}
float eraserHitRadius(float strokeRadius, float eraserRadius)
{
    return std::max(2.0f, eraserRadius + strokeRadius * 0.35f);
}
void keepLocalEraserGap(std::vector<bool>& hitFlags,
                        std::size_t closestIndex,
                        float spacing,
                        float eraserRadius,
                        float strokeRadius)
{
    if (hitFlags.empty()) {
        return;
    }
    std::fill(hitFlags.begin(), hitFlags.end(), false);
    const float localGapRadius = std::max(2.0f, eraserRadius * 1.15f + strokeRadius * 0.25f);
    const int gapSamples = std::max(1, static_cast<int>(std::ceil(localGapRadius / std::max(0.5f, spacing))));
    const int begin = std::max(0, static_cast<int>(closestIndex) - gapSamples);
    const int end = std::min(static_cast<int>(hitFlags.size()) - 1,
                             static_cast<int>(closestIndex) + gapSamples);
    for (int i = begin; i <= end; ++i) {
        hitFlags[static_cast<std::size_t>(i)] = true;
    }
}
void appendStrokePart(std::vector<Stroke>& output, const Stroke& source, std::vector<StrokePoint>& points)
{
    if (points.empty()) {
        return;
    }
    Stroke part = source;
    part.points = points;
    output.push_back(std::move(part));
    points.clear();
}
std::vector<Stroke> splitStrokeByEraser(const Stroke& stroke,
                                        const Stroke& eraserStroke,
                                        float eraserRadius,
                                        bool& changed)
{
    std::vector<Stroke> result;
    if (stroke.points.empty() || eraserStroke.points.empty()) {
        result.push_back(stroke);
        return result;
    }
    const float spacing = std::max(1.0f, std::min(stroke.radiusPx, eraserRadius) * 0.5f);
    const std::vector<StrokePoint> sampledPoints = resampleStrokeForEraser(stroke, spacing);
    if (sampledPoints.empty()) {
        result.push_back(stroke);
        return result;
    }
    const float hitRadius = eraserHitRadius(stroke.radiusPx, eraserRadius);
    const float hitRadiusSq = hitRadius * hitRadius;
    std::vector<bool> hitFlags(sampledPoints.size(), false);
    int hitCount = 0;
    float bestDistanceSq = std::numeric_limits<float>::infinity();
    std::size_t closestIndex = 0U;
    for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
        const float distanceSq = distancePointToStrokePathSquared(sampledPoints[i], eraserStroke);
        if (distanceSq < bestDistanceSq) {
            bestDistanceSq = distanceSq;
            closestIndex = i;
        }
        if (distanceSq <= hitRadiusSq) {
            hitFlags[i] = true;
            ++hitCount;
        }
    }
    if (hitCount == 0) {
        result.push_back(stroke);
        return result;
    }
    const float targetLength = strokeApproxLength(stroke);
    const float eraserPathLength = strokeApproxLength(eraserStroke);
    const bool shortEraserGesture = eraserPathLength <= std::max(4.0f, eraserRadius * 1.5f);
    const bool mostlyHit = hitCount >= static_cast<int>(sampledPoints.size() * 0.90f);
    if (shortEraserGesture && sampledPoints.size() >= 3U &&
        targetLength > std::max(8.0f, eraserRadius * 2.5f) && mostlyHit) {
        keepLocalEraserGap(hitFlags, closestIndex, spacing, eraserRadius, stroke.radiusPx);
    }
    std::vector<StrokePoint> currentPart;
    bool anyErasedPoint = false;
    for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
        if (hitFlags[i]) {
            anyErasedPoint = true;
            appendStrokePart(result, stroke, currentPart);
        } else {
            currentPart.push_back(sampledPoints[i]);
        }
    }
    appendStrokePart(result, stroke, currentPart);
    if (anyErasedPoint && result.empty() && shortEraserGesture) {
        keepLocalEraserGap(hitFlags, closestIndex, spacing, eraserRadius, stroke.radiusPx);
        currentPart.clear();
        for (std::size_t i = 0; i < sampledPoints.size(); ++i) {
            if (hitFlags[i]) {
                appendStrokePart(result, stroke, currentPart);
            } else {
                currentPart.push_back(sampledPoints[i]);
            }
        }
        appendStrokePart(result, stroke, currentPart);
    }
    if (!anyErasedPoint || result.empty()) {
        result.clear();
        result.push_back(stroke);
        return result;
    }
    changed = true;
    return result;
}
ImU32 onionColor(bool isPrevious, float opacity)
{
    const int alpha = static_cast<int>(std::lround(std::clamp(opacity, 0.0f, 1.0f) * 255.0f));
    return isPrevious ? IM_COL32(35, 170, 255, alpha) : IM_COL32(255, 95, 70, alpha);
}
void drawOnionStrokeDirect(const Stroke& stroke, bool isPrevious, float opacity, const CanvasView& view, ImVec2 areaMin, ImVec2 areaSize, ImDrawList* drawList)
{
    if (drawList == nullptr || stroke.points.empty() || opacity <= 0.0f) {
        return;
    }
    const ImU32 color = onionColor(isPrevious, opacity * std::max(0.35f, stroke.color[3]));
    const float zoom = std::clamp(view.zoom, 0.05f, 32.0f);
    const float width = std::clamp(stroke.radiusPx * zoom * 0.55f, 1.2f, 4.0f);
    if (stroke.points.size() == 1U) {
        const StrokePoint& point = stroke.points.front();
        const ImVec2 p = view.canvasToScreen(point.x, point.y, areaMin, areaSize);
        drawList->AddCircleFilled(p, width * 0.5f, color, 12);
        return;
    }
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& previous = stroke.points[index - 1U];
        const StrokePoint& current = stroke.points[index];
        const ImVec2 p0 = view.canvasToScreen(previous.x, previous.y, areaMin, areaSize);
        const ImVec2 p1 = view.canvasToScreen(current.x, current.y, areaMin, areaSize);
        const float pressure = std::max(0.1f, (previous.pressure + current.pressure) * 0.5f);
        drawList->AddLine(p0, p1, color, std::clamp(width * pressure, 1.2f, 4.0f));
    }
}
void drawOnionFrameDirect(const Frame& frame, bool isPrevious, float opacity, const CanvasView& view, ImVec2 areaMin, ImVec2 areaSize, ImDrawList* drawList)
{
    if (drawList == nullptr || opacity <= 0.0f) {
        return;
    }
    const ImVec2 areaMax(areaMin.x + areaSize.x, areaMin.y + areaSize.y);
    drawList->PushClipRect(areaMin, areaMax, true);
    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f) {
            continue;
        }
        for (const Stroke& stroke : layer.strokes) {
            drawOnionStrokeDirect(stroke, isPrevious, opacity * layer.opacity, view, areaMin, areaSize, drawList);
        }
    }
    drawList->PopClipRect();
}

void drawLightweightEraserPreview(const Stroke& eraserStroke,
                                  const CanvasView& view,
                                  ImVec2 areaMin,
                                  ImVec2 areaSize,
                                  ImDrawList* drawList)
{
    if (drawList == nullptr || eraserStroke.points.empty()) {
        return;
    }

    // 消しゴムプレビューは、確定後の全ストローク分割を毎フレーム再計算しない。
    // 代わりに現在なぞっている軌跡だけをキャンバス背景色で上から薄く塗り、
    // 「このあたりが消える」という結果を軽く見せる。
    const float zoom = std::clamp(view.zoom, 0.05f, 32.0f);
    const float radius = std::max(2.0f, eraserStroke.radiusPx * zoom);
    const float strokeWidth = radius * 2.0f;

    ImVec4 maskColor = ui::themeColors().canvasBackground;
    maskColor.w = 0.92f;
    const ImU32 mask = ImGui::ColorConvertFloat4ToU32(maskColor);
    const ImU32 guide = IM_COL32(255, 80, 70, 145);

    const ImVec2 areaMax(areaMin.x + areaSize.x, areaMin.y + areaSize.y);
    drawList->PushClipRect(areaMin, areaMax, true);

    const auto toScreen = [&](const StrokePoint& point) {
        return view.canvasToScreen(point.x, point.y, areaMin, areaSize);
    };

    if (eraserStroke.points.size() == 1U) {
        const ImVec2 p = toScreen(eraserStroke.points.front());
        drawList->AddCircleFilled(p, radius, mask, 28);
        drawList->AddCircle(p, radius, guide, 28, 1.5f);
        drawList->PopClipRect();
        return;
    }

    ImVec2 previous = toScreen(eraserStroke.points.front());
    drawList->AddCircleFilled(previous, radius, mask, 24);
    for (std::size_t i = 1; i < eraserStroke.points.size(); ++i) {
        const ImVec2 current = toScreen(eraserStroke.points[i]);
        drawList->AddLine(previous, current, mask, strokeWidth);
        drawList->AddCircleFilled(current, radius, mask, 24);
        previous = current;
    }

    previous = toScreen(eraserStroke.points.front());
    for (std::size_t i = 1; i < eraserStroke.points.size(); ++i) {
        const ImVec2 current = toScreen(eraserStroke.points[i]);
        drawList->AddLine(previous, current, guide, 1.5f);
        previous = current;
    }

    drawList->PopClipRect();
}
Frame previewFrameWithEraser(const Frame& frame, int activeLayerIndex, const Stroke& eraserStroke)
{
    Frame preview = frame;
    if (eraserStroke.points.empty() || activeLayerIndex < 0 || activeLayerIndex >= static_cast<int>(preview.layers.size())) {
        return preview;
    }
    Layer& layer = preview.layers[static_cast<std::size_t>(activeLayerIndex)];
    std::vector<Stroke> rewrittenStrokes;
    rewrittenStrokes.reserve(layer.strokes.size());
    bool changed = false;
    const float radius = std::max(1.0f, eraserStroke.radiusPx);
    for (const Stroke& stroke : layer.strokes) {
        std::vector<Stroke> parts = splitStrokeByEraser(stroke, eraserStroke, radius, changed);
        rewrittenStrokes.insert(rewrittenStrokes.end(),
                                std::make_move_iterator(parts.begin()),
                                std::make_move_iterator(parts.end()));
    }
    if (changed) {
        layer.strokes = std::move(rewrittenStrokes);
    }
    return preview;
}
} // namespace
void App::drawDrawingMode()
{
    // Timesheet Rebuild Step 6:
    // TimesheetPanelの一時入力をApp内の正式core Timesheetへ同期し、
    // T選択をキャンバス表示プレビューへ反映する。
    // まだ再生、出力、Project保存の自動連動には反映しない。
    {
        timesheetPanelState_.showDetachedWindow = true;

        ::perapera::ui::TimesheetPanelViewModel timesheetPanelData;
        timesheetPanelData.totalFrames = project_.timeline.totalFrames > 0 ? project_.timeline.totalFrames : 1;
        timesheetPanelData.fps = 24;
        timesheetPanelData.cells.reserve(project_.cells.size());

        for (const auto& timesheetCell : project_.cells) {
            ::perapera::ui::TimesheetPanelCellColumn column;
            column.cellId = timesheetCell.id;
            column.displayName = timesheetCell.name;
            column.category = timesheetCell.category;
            timesheetPanelData.cells.push_back(column);
        }

        // Timesheet Rebuild Step 5.6:
        // 現在のセル列・総フレーム数に合わせて、UI一時入力を先に正規化する。
        // セル削除後の古いcellId、範囲外T、重複entryを残したまま保存・変換しない。
        const bool timesheetPanelStateNormalized =
            ::perapera::ui::normalizeTimesheetPanelStateForViewModel(timesheetPanelData, timesheetPanelState_);
        if (timesheetPanelStateNormalized) {
            workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
            workingTimesheetDirty_ = true;
            lastMessage_ = "timesheet panel state normalized: entries=" +
                std::to_string(countTimesheetEntries(workingTimesheet_));
        }

        if (workingTimesheet_.tracks.empty() || !workingTimesheetDirty_) {
            workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
        } else {
            workingTimesheet_.totalFrames = std::max(1, timesheetPanelData.totalFrames);
        }

        // Timesheet Rebuild Step 6.5:
        // タイムシートTだけを進める試作再生。作画F編集対象 activeFrameIndex_ は変更しない。
        auto setTimesheetPreviewFrame = [&](int zeroBasedFrame, const char* reason) {
            const int lastFrame = std::max(0, timesheetPanelData.totalFrames - 1);
            const int nextFrame = std::clamp(zeroBasedFrame, 0, lastFrame);
            if (timesheetPanelState_.selectedTimelineFrame != nextFrame) {
                timesheetPanelState_.selectedTimelineFrame = nextFrame;
                canvasRenderer_.markAllDirty();
                lastMessage_ = std::string(reason) + " T" + std::to_string(nextFrame + 1) +
                    " / edit F" + std::to_string(activeFrameIndex_ + 1);
            }
        };

        auto stepTimesheetPreviewFrame = [&](int delta) {
            const int frameCount = std::max(1, timesheetPanelData.totalFrames);
            if (frameCount <= 1) {
                isPlayingTimesheet_ = false;
                timesheetPlaybackAccumulator_ = 0.0f;
                return;
            }

            int nextFrame = timesheetPanelState_.selectedTimelineFrame + delta;
            if (nextFrame < 0 || nextFrame >= frameCount) {
                if (timesheetPlaybackPingPong_) {
                    timesheetPlaybackDirection_ = -timesheetPlaybackDirection_;
                    nextFrame = timesheetPanelState_.selectedTimelineFrame + timesheetPlaybackDirection_;
                } else {
                    nextFrame = nextFrame >= frameCount ? 0 : frameCount - 1;
                }
            }
            setTimesheetPreviewFrame(nextFrame, "timesheet playback");
        };

        if (isPlayingFrames_ && isPlayingTimesheet_) {
            isPlayingTimesheet_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
        }

        ImGui::Begin(
            u8c(u8"タイムシート再生##FormalTimesheetPlayback"),
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::TextUnformatted(u8c(u8"Step 6.5: タイムシートT再生の試作"));
        ImGui::Text(
            u8c(u8"T %d / %d   entries=%d"),
            timesheetPanelState_.selectedTimelineFrame + 1,
            std::max(1, timesheetPanelData.totalFrames),
            countTimesheetEntries(workingTimesheet_));
        if (ImGui::SmallButton("|<##TimesheetPlaybackFirst")) {
            isPlayingTimesheet_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            setTimesheetPreviewFrame(0, "timesheet preview");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("<##TimesheetPlaybackPrev")) {
            isPlayingTimesheet_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            stepTimesheetPreviewFrame(-1);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(isPlayingTimesheet_ ? u8c(u8"停止##TimesheetPlaybackStop") : u8c(u8"再生##TimesheetPlaybackPlay"))) {
            isPlayingTimesheet_ = !isPlayingTimesheet_;
            isPlayingFrames_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            lastMessage_ = isPlayingTimesheet_ ? "timesheet playback started" : "timesheet playback stopped";
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(">##TimesheetPlaybackNext")) {
            isPlayingTimesheet_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            stepTimesheetPreviewFrame(1);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(">|##TimesheetPlaybackLast")) {
            isPlayingTimesheet_ = false;
            timesheetPlaybackAccumulator_ = 0.0f;
            setTimesheetPreviewFrame(timesheetPanelData.totalFrames - 1, "timesheet preview");
        }

        const char* timesheetSpeedItems[] = {"0.25x", "0.5x", "1x", "2x"};
        int timesheetSpeedIndex = 2;
        if (timesheetPlaybackSpeed_ <= 0.26f) {
            timesheetSpeedIndex = 0;
        } else if (timesheetPlaybackSpeed_ <= 0.51f) {
            timesheetSpeedIndex = 1;
        } else if (timesheetPlaybackSpeed_ >= 1.99f) {
            timesheetSpeedIndex = 3;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        if (ImGui::Combo(u8c(u8"速度##TimesheetPlaybackSpeed"), &timesheetSpeedIndex, timesheetSpeedItems, IM_ARRAYSIZE(timesheetSpeedItems))) {
            const float timesheetSpeeds[] = {0.25f, 0.5f, 1.0f, 2.0f};
            timesheetPlaybackSpeed_ = timesheetSpeeds[std::clamp(timesheetSpeedIndex, 0, 3)];
            timesheetPlaybackAccumulator_ = 0.0f;
        }
        ImGui::SameLine();
        ImGui::Checkbox(u8c(u8"ピンポン##TimesheetPlaybackPingPong"), &timesheetPlaybackPingPong_);
        ImGui::TextDisabled(u8c(u8"タイムシート再生はTだけを進めます。作画F編集対象は変更しません。"));
        ImGui::TextDisabled(u8c(u8"指パラ/通常タイムライン再生中は、作画F再生表示を優先します。"));
        ImGui::End();

        if (isPlayingTimesheet_) {
            if (isDrawingStroke_ || timesheetPanelData.totalFrames <= 1) {
                isPlayingTimesheet_ = false;
                timesheetPlaybackAccumulator_ = 0.0f;
            } else {
                const float playbackFps = static_cast<float>(std::max(1, timesheetPanelData.fps));
                const float secondsPerTimelineFrame = std::max(1.0f / 60.0f, 1.0f / (playbackFps * std::max(0.01f, timesheetPlaybackSpeed_)));
                timesheetPlaybackAccumulator_ += std::clamp(ImGui::GetIO().DeltaTime, 0.0f, 0.1f);
                while (timesheetPlaybackAccumulator_ >= secondsPerTimelineFrame) {
                    timesheetPlaybackAccumulator_ -= secondsPerTimelineFrame;
                    const int direction = timesheetPlaybackDirection_ == 0 ? 1 : timesheetPlaybackDirection_;
                    stepTimesheetPreviewFrame(direction);
                }
            }
        } else {
            timesheetPlaybackAccumulator_ = 0.0f;
        }

        if (!timesheetPanelState_.windowOpen) {
            ImGui::Begin(
                u8c(u8"タイムシートを開く##FormalTimesheetReopen"),
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::TextUnformatted(u8c(u8"タイムシートは閉じています。"));
            if (ImGui::SmallButton(u8c(u8"タイムシートを開く"))) {
                timesheetPanelState_.windowOpen = true;
            }
            ImGui::End();
        }

        const ::perapera::ui::TimesheetPanelResult timesheetPanelResult =
            ::perapera::ui::drawTimesheetPanel(timesheetPanelData, timesheetPanelState_);

        if (timesheetPanelResult.entryChanged) {
            workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
            workingTimesheetDirty_ = true;
            canvasRenderer_.markAllDirty();

            const int entryCount = countTimesheetEntries(workingTimesheet_);
            lastMessage_ = "timesheet temporary core data updated: entries=" + std::to_string(entryCount);
        }

        if (timesheetPanelResult.timelineFrameChanged) {
            canvasRenderer_.markAllDirty();
            lastMessage_ = "timesheet preview T" + std::to_string(timesheetPanelState_.selectedTimelineFrame + 1) +
                " / edit F" + std::to_string(activeFrameIndex_ + 1);
        }

        // Timesheet Rebuild Step 5.5:
        // まだProject保存とは統合せず、現在のプロジェクトフォルダ直下 timesheet.json へ
        // 明示ボタンで一時Timesheetを保存/読み込みする。
        const std::filesystem::path projectFolder = appio::absolutePath(exportState_.projectFolder);
        const std::filesystem::path timesheetPath = TimesheetIO::timesheetPathForCutFolder(projectFolder);

        ImGui::Begin(
            u8c(u8"タイムシート保存##FormalTimesheetManualIO"),
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::TextUnformatted(u8c(u8"Step 5.5: 一時タイムシートの手動保存/読み込み"));
        ImGui::TextWrapped(
            u8c(u8"保存先: %s"),
            timesheetPath.string().c_str());
        ImGui::Text(
            u8c(u8"一時Timesheet: entries=%d%s"),
            countTimesheetEntries(workingTimesheet_),
            workingTimesheetDirty_ ? u8c(u8" / 未保存の可能性") : u8c(u8""));

        if (ImGui::SmallButton(u8c(u8"一時タイムシート保存"))) {
            workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
            workingTimesheet_.totalFrames = std::max(1, timesheetPanelData.totalFrames);

            std::string error;
            if (TimesheetIO::saveTimesheet(workingTimesheet_, timesheetPath, &error)) {
                workingTimesheetDirty_ = false;
                lastMessage_ = "timesheet saved: " + timesheetPath.string() +
                    " | entries=" + std::to_string(countTimesheetEntries(workingTimesheet_));
            } else {
                lastMessage_ = "timesheet save failed: " + error;
            }
        }

        ImGui::SameLine();
        if (ImGui::SmallButton(u8c(u8"一時タイムシート読込"))) {
            Timesheet loadedTimesheet;
            std::string error;
            if (TimesheetIO::loadTimesheet(timesheetPath, loadedTimesheet, &error)) {
                workingTimesheet_ = std::move(loadedTimesheet);
                ::perapera::ui::replacePanelEntriesFromTimesheet(workingTimesheet_, timesheetPanelState_);
                const bool normalizedLoadedPanel =
                    ::perapera::ui::normalizeTimesheetPanelStateForViewModel(timesheetPanelData, timesheetPanelState_);
                if (normalizedLoadedPanel) {
                    workingTimesheet_ = ::perapera::ui::buildTimesheetFromPanelState(timesheetPanelData, timesheetPanelState_);
                }
                workingTimesheetDirty_ = normalizedLoadedPanel;
                lastMessage_ = "timesheet loaded: " + timesheetPath.string() +
                    " | entries=" + std::to_string(countTimesheetEntries(workingTimesheet_));
            } else {
                lastMessage_ = "timesheet load failed: " + error;
            }
        }

        ImGui::TextDisabled(u8c(u8"この保存は試験段階です。Project保存・キャンバス表示・再生・出力にはまだ接続していません。"));
        ImGui::End();

        // Timesheet Rebuild Step 7.4:
        // Tを選んだとき、各セルがどの作画Fとして解決されているかを小さい一覧で見せる。
        // 「Tを選んでも何が表示されたのか分からない」を潰すための確認用UI。
        const int resolvedTimelineFrame = clampTimesheetFrame(workingTimesheet_, timesheetPanelState_.selectedTimelineFrame + 1);
        const ui::CellDisplayMode timesheetSummaryDisplayMode = ui::currentCellDisplayMode();
        const int timesheetSummarySoloCellIndex = ui::currentSoloCellIndex();
        std::vector<std::pair<int, const Cell*>> timesheetSummaryCells;

        auto appendTimesheetSummaryCell = [&](int cellIndex, const Cell* cell) {
            if (cell == nullptr) {
                return;
            }
            timesheetSummaryCells.push_back(std::make_pair(cellIndex, cell));
        };

        if (timesheetSummaryDisplayMode == ui::CellDisplayMode::ActiveOnly || project_.cells.empty()) {
            appendTimesheetSummaryCell(activeCellIndex_, activeCell());
        } else if (timesheetSummaryDisplayMode == ui::CellDisplayMode::SoloSelected) {
            const int safeSoloIndex = std::clamp(
                timesheetSummarySoloCellIndex >= 0 ? timesheetSummarySoloCellIndex : activeCellIndex_,
                0,
                static_cast<int>(project_.cells.size()) - 1);
            appendTimesheetSummaryCell(safeSoloIndex, &project_.cells[static_cast<std::size_t>(safeSoloIndex)]);
        } else {
            timesheetSummaryCells.reserve(project_.cells.size());
            for (int cellIndex = 0; cellIndex < static_cast<int>(project_.cells.size()); ++cellIndex) {
                appendTimesheetSummaryCell(cellIndex, &project_.cells[static_cast<std::size_t>(cellIndex)]);
            }
        }

        ImGui::Begin(
            u8c(u8"タイムシート解決結果##FormalTimesheetResolvedList"),
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Text(
            u8c(u8"T%d の表示結果 / 編集対象: 作画F%d / entries=%d"),
            resolvedTimelineFrame,
            activeFrameIndex_ + 1,
            countTimesheetEntries(workingTimesheet_));
        ImGui::TextDisabled(u8c(u8"T=タイムシート位置 / 作画F=実際に描く絵の番号 / コマ数=時間の長さ"));

        int resolvedVisibleCount = 0;
        if (ImGui::BeginTable(
                "TimesheetResolvedListTable_v27",
                5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(u8c(u8"セル"));
            ImGui::TableSetupColumn(u8c(u8"セル状態"));
            ImGui::TableSetupColumn(u8c(u8"記入/保持"));
            ImGui::TableSetupColumn(u8c(u8"表示"));
            ImGui::TableSetupColumn(u8c(u8"備考"));
            ImGui::TableHeadersRow();

            for (const auto& cellPair : timesheetSummaryCells) {
                const int cellIndex = cellPair.first;
                const Cell* cell = cellPair.second;
                if (cell == nullptr) {
                    continue;
                }

                const ResolvedTimesheetCell resolved = resolveTimesheetCell(workingTimesheet_, cell->id, resolvedTimelineFrame);
                const bool cellEnabledForAllDisplay = cell->visible && cell->opacity > 0.0f;
                const bool displayModeKeepsHiddenCell = timesheetSummaryDisplayMode == ui::CellDisplayMode::ActiveOnly ||
                    timesheetSummaryDisplayMode == ui::CellDisplayMode::SoloSelected;
                const bool cellCanAppear = cellEnabledForAllDisplay || displayModeKeepsHiddenCell;
                const bool frameExists = resolved.drawingFrameNumber > 0 &&
                    resolved.drawingFrameNumber <= static_cast<int>(cell->frames.size());
                const bool visibleOnCanvas = cellCanAppear && resolved.visible && frameExists;
                if (visibleOnCanvas) {
                    ++resolvedVisibleCount;
                }

                const char* sourceLabel = resolved.sourceEntry == nullptr
                    ? u8c(u8"未記入")
                    : timesheetEntryTypeJapaneseLabel(resolved.sourceType);
                const std::string cellLabel = cell->name.empty()
                    ? std::string(u8c(u8"セル")) + std::to_string(cellIndex + 1)
                    : cell->name;
                const std::string cellState = cellEnabledForAllDisplay
                    ? std::string(u8c(u8"ON"))
                    : std::string(u8c(u8"セルOFF/不透明度0"));
                std::string displayLabel = u8c(u8"非表示");
                if (visibleOnCanvas) {
                    displayLabel = std::string(u8c(u8"作画F")) + std::to_string(resolved.drawingFrameNumber);
                } else if (resolved.visible && !frameExists) {
                    displayLabel = std::string(u8c(u8"作画F")) + std::to_string(resolved.drawingFrameNumber) + u8c(u8" 範囲外");
                }

                std::string note;
                if (resolved.sourceEntry == nullptr) {
                    note = u8c(u8"このT以前に記入なし");
                } else if (resolved.sourceType == TimesheetEntryType::Hold) {
                    note = u8c(u8"前の表示を保持");
                } else if (resolved.sourceType == TimesheetEntryType::Null) {
                    note = u8c(u8"空セルで非表示");
                } else if (!cellCanAppear) {
                    note = u8c(u8"セル表示OFFのため非表示");
                } else if (resolved.visible && !frameExists) {
                    note = u8c(u8"セル内にその作画Fがない");
                }

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(cellLabel.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(cellState.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(sourceLabel);
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(displayLabel.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(note.c_str());
            }
            ImGui::EndTable();
        }

        ImGui::Text(
            u8c(u8"表示セルN=%d / 候補セル=%d"),
            resolvedVisibleCount,
            static_cast<int>(timesheetSummaryCells.size()));
        ImGui::TextDisabled(u8c(u8"この一覧は確認用です。編集対象の作画Fはここでは変更しません。"));
        ImGui::End();

        // Timesheet Rebuild Step 7.5:
        // 選択T/セルについて、前後の原画とその間の中割を検出する。
        // ここではまだ中割を自動配置せず、次のStep 7.6のための確認UIに留める。
        if (!timesheetPanelData.cells.empty()) {
            const int safeSelectedCellColumn = std::clamp(
                timesheetPanelState_.selectedCellColumn,
                0,
                static_cast<int>(timesheetPanelData.cells.size()) - 1);
            const ::perapera::ui::TimesheetPanelCellColumn& selectedColumn =
                timesheetPanelData.cells[static_cast<std::size_t>(safeSelectedCellColumn)];
            const ResolvedTimesheetInbetweenInterval interval = resolveTimesheetInbetweenInterval(
                workingTimesheet_,
                selectedColumn.cellId,
                resolvedTimelineFrame);

            ImGui::Begin(
                u8c(u8"原画間検出##FormalTimesheetInbetweenInterval"),
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Text(
                u8c(u8"対象: %s / T%d"),
                selectedColumn.displayName.empty() ? selectedColumn.cellId.c_str() : selectedColumn.displayName.c_str(),
                interval.selectedTimelineFrame);
            ImGui::TextDisabled(u8c(u8"原画を先に置き、その間に中割を作るための検出結果です。"));

            if (!interval.previousKey.found && !interval.nextKey.found) {
                ImGui::TextUnformatted(u8c(u8"このセルにはまだ原画がありません。まずTに原画を置いてください。"));
            } else {
                const std::string previousKeyLabel = interval.previousKey.found
                    ? std::string(u8c(u8"T")) + std::to_string(interval.previousKey.timelineFrame) +
                        std::string(u8c(u8" / 作画F")) + std::to_string(interval.previousKey.drawingFrameNumber)
                    : std::string(u8c(u8"なし"));
                const std::string nextKeyLabel = interval.nextKey.found
                    ? std::string(u8c(u8"T")) + std::to_string(interval.nextKey.timelineFrame) +
                        std::string(u8c(u8" / 作画F")) + std::to_string(interval.nextKey.drawingFrameNumber)
                    : std::string(u8c(u8"なし"));

                if (ImGui::BeginTable(
                        "TimesheetInbetweenIntervalTable_v1",
                        2,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn(u8c(u8"項目"));
                    ImGui::TableSetupColumn(u8c(u8"内容"));
                    ImGui::TableHeadersRow();

                    auto tableRow = [](const char* label, const std::string& value) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextUnformatted(label);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextUnformatted(value.c_str());
                    };

                    tableRow(u8c(u8"前原画"), previousKeyLabel);
                    tableRow(u8c(u8"次原画"), nextKeyLabel);
                    tableRow(
                        u8c(u8"選択T"),
                        interval.selectedIsKey
                            ? std::string(u8c(u8"原画"))
                            : (interval.selectedIsInbetween
                                ? std::string(u8c(u8"中割"))
                                : (interval.selectedInsideInterval
                                    ? std::string(u8c(u8"原画間の空きT"))
                                    : std::string(u8c(u8"原画間の外")))));
                    tableRow(
                        u8c(u8"原画間位置"),
                        interval.hasClosedKeyInterval
                            ? std::to_string(interval.selectedPositionNumerator) + "/" + std::to_string(interval.selectedPositionDenominator)
                            : std::string(u8c(u8"未確定")));
                    tableRow(
                        u8c(u8"中割"),
                        std::to_string(interval.usedInbetweenCount) +
                            std::string(u8c(u8"枚 / 配置候補T")) +
                            std::to_string(interval.availableTimelineSlotCount));
                    ImGui::EndTable();
                }

                if (interval.hasClosedKeyInterval) {
                    if (interval.inbetweens.empty()) {
                        ImGui::TextUnformatted(u8c(u8"この原画間にはまだ中割がありません。"));
                    } else {
                        ImGui::TextUnformatted(u8c(u8"この原画間の中割:"));
                        for (const TimesheetInbetweenDrawingInfo& inbetween : interval.inbetweens) {
                            ImGui::BulletText(
                                u8c(u8"%d/%d: T%d / 作画F%d"),
                                inbetween.inbetweenIndex,
                                inbetween.inbetweenCount,
                                inbetween.timelineFrame,
                                inbetween.drawingFrameNumber);
                        }
                    }
                    ImGui::TextDisabled(u8c(u8"Step 7.6で、この区間に中割を1枚/2枚/3枚追加するボタンを入れます。"));
                } else {
                    ImGui::TextDisabled(u8c(u8"前後原画が両方あると、中割作成候補として扱えます。"));
                }
            }
            ImGui::TextDisabled(u8c(u8"絵コンテ・レイアウト・背景参照はセル列に混ぜず、Scene Plateとして別管理します。"));
            ImGui::End();
        }
    }
    const bool isColoringMode = currentMode_ == AppMode::Coloring;
    if (isColoringMode) {
        selectPaintLayerForColoring(true);
    }
    clampSelection();
    handleFrameShortcuts();
    updateFramePlayback();
    // Timesheet Rebuild Step 7.3:
    // キャンバス上ホイール時に親ワークスペースが縦スクロールしないようにする。
    ImGui::BeginChild("DrawingWorkspace",
                      ImVec2(0.0f, -28.0f),
                      true,
                      ImGuiWindowFlags_NoScrollWithMouse);
    const float sideWidth = 245.0f;
    const float rightWidth = 315.0f;
    // Timesheet Rebuild Step 7.3:
    // 下部タイムラインに作画Fの四角列を確実に出すため、高さを少し増やす。
    const float timelineHeight = 215.0f;
    const float centerHeight = std::max(160.0f,
                                        ImGui::GetContentRegionAvail().y - timelineHeight - ImGui::GetStyle().ItemSpacing.y);
    ImGui::BeginChild("DrawingUpperArea",
                      ImVec2(0.0f, centerHeight),
                      false,
                      ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::BeginChild("DrawingLeftSidebar", ImVec2(sideWidth, 0.0f), true);
    drawLeftSidebar();
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("DrawingCanvasHost",
                      ImVec2(-(rightWidth + ImGui::GetStyle().ItemSpacing.x), 0.0f),
                      true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    drawCanvasArea(rightWidth);
    ImGui::SetScrollX(0.0f);
    ImGui::SetScrollY(0.0f);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("DrawingRightSidebar_v5", ImVec2(rightWidth, 0.0f), true,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushID("DrawingRightSidebar_v5Content_v4");
    drawRightSidebar();
    ImGui::PopID();
    ImGui::EndChild();
    ImGui::EndChild();
    drawTimelineArea();
    ImGui::EndChild();
}
void App::drawLeftSidebar()
{
    if (currentMode_ == AppMode::Coloring) {
        ImGui::TextUnformatted(u8c(u8"③.5 彩色モード"));
        ImGui::TextWrapped(u8c(u8"Paintレイヤーを編集対象にし、Normal/ColorTrace/ShadowGuideを参照表示します。"));
        if (ImGui::Button(u8c(u8"Paintレイヤーへ移動"), ImVec2(-1.0f, 0.0f))) {
            selectPaintLayerForColoring(true);
        }
        ImGui::Separator();
    }

    ui::drawBrushPanel(brushSettings_);
    ImGui::Separator();
    ui::drawColorPanel(colorPanelState_, brushSettings_);
    ImGui::Separator();
    ImGui::TextUnformatted(u8c(u8"操作"));
    if (ImGui::Button("Undo", ImVec2(-1.0f, 0.0f))) {
        undo();
    }
    if (ImGui::Button("Redo", ImVec2(-1.0f, 0.0f))) {
        redo();
    }
    if (ImGui::Button(u8c(u8"全体表示"), ImVec2(-1.0f, 0.0f))) {
        canvasViewInitialized_ = false;
    }
    ImGui::TextWrapped(u8c(u8"左ドラッグ: 描く/消す\n中ボタンドラッグ: パン\nホイール: ズーム"));
}
void App::drawRightSidebar()
{
    const ui::CellPanelResult cellPanelResult = ui::drawCellPanel(project_, activeCellIndex_);
    const auto resetPreviewReadiness = [&]() {
        previewWarmCursor_ = activeFrameIndex_;
        previewReadyFlags_.clear();
        previewReadyCount_ = 0;
        previewReadyScanCursor_ = activeFrameIndex_;
    };
    if (cellPanelResult.projectStructureChanged) {
        activeCellIndex_ = cellPanelResult.selectedCellIndex;
        clampSelection();
        resetPreviewReadiness();
        canvasRenderer_.clearLayerCaches();
        lastMessage_ = cellPanelResult.selectionChanged ? "active cell changed" : "cell structure changed";
    } else if (cellPanelResult.selectionChanged) {
        activeCellIndex_ = cellPanelResult.selectedCellIndex;
        clampSelection();
        resetPreviewReadiness();
        canvasRenderer_.markAllDirty();
        lastMessage_ = "active cell changed";
    } else if (cellPanelResult.displayChanged) {
        canvasRenderer_.markAllDirty();
        lastMessage_ = "cell display changed";
    }

    ImGui::Separator();

    Frame* frame = activeFrame();
    Cell* cell = activeCell();

    if (frame == nullptr || cell == nullptr) {
        ImGui::TextUnformatted("No active cell or frame.");
        return;
    }
    ImGui::TextDisabled(currentMode_ == AppMode::Coloring ? "Phase 2-pre CellPanel v1 coloring mode" : "Phase 2-pre CellPanel v1 drawing mode");
    const ui::LayerPanelAction layerAction = ui::drawLayerPanel(*frame, activeLayerIndex_);
    if (layerAction == ui::LayerPanelAction::AddLayer) {
        addLayer();
    } else if (layerAction == ui::LayerPanelAction::DeleteLayer) {
        deleteActiveLayer();
    } else if (layerAction == ui::LayerPanelAction::ClearLayer) {
        clearActiveLayer();
    }
    ImGui::Separator();
    const ui::FramePanelAction frameAction = ui::drawFramePanel(*cell, activeFrameIndex_);
    if (frameAction == ui::FramePanelAction::AddFrame) {
        addFrame();
    } else if (frameAction == ui::FramePanelAction::DuplicateFrame) {
        duplicateFrame();
    } else if (frameAction == ui::FramePanelAction::DeleteFrame) {
        deleteActiveFrame();
    }
    ImGui::Separator();
    const ui::ExportPanelAction exportAction = ui::drawExportPanel(exportState_, lastMessage_.c_str());
    if (exportAction == ui::ExportPanelAction::SaveProject) {
        saveProject();
    } else if (exportAction == ui::ExportPanelAction::LoadProject) {
        loadProject();
    } else if (exportAction == ui::ExportPanelAction::VerifyProjectRoundTrip) {
        saveLoadRoundTripCheck();
    } else if (exportAction == ui::ExportPanelAction::ExportActivePng) {
        exportActivePng();
    } else if (exportAction == ui::ExportPanelAction::ExportPngSequence) {
        exportPngSequence();
    } else if (exportAction == ui::ExportPanelAction::ExportMp4) {
        exportMp4();
    }
}

void App::drawTimelineArea()
{
    Cell* cell = activeCell();
    if (cell == nullptr) {
        return;
    }

    // Timesheet Rebuild Step 7.1:
    // 下部タイムラインは、フレーム列を先に見せる。
    // ライトテーブル/指パラ操作はその下へ移し、視線がフレーム列へ先に行くようにする。
    ImGui::BeginChild("DrawingTimelineFrameStripHost_v26", ImVec2(0.0f, 160.0f), true,
                      ImGuiWindowFlags_NoScrollWithMouse);
    const int prevFrameIndex = activeFrameIndex_;
    const ui::TimelinePanelAction timelineAction =
        ui::drawTimelinePanel(*cell, activeFrameIndex_, onionPrevious_, onionNext_);
    if (activeFrameIndex_ != prevFrameIndex) {
        canvasRenderer_.markAllDirty();
    }
    if (timelineAction == ui::TimelinePanelAction::AddFrame) {
        addFrame();
    } else if (timelineAction == ui::TimelinePanelAction::DuplicateFrame) {
        duplicateFrame();
    } else if (timelineAction == ui::TimelinePanelAction::DeleteFrame) {
        deleteActiveFrame();
    }
    ImGui::EndChild();

    ImGui::Separator();
    drawFingerPlaybackControls();
    drawLightTableControls();
}
void App::drawCanvasArea(float rightWidth)
{
    (void)rightWidth;
    Frame* frame = activeFrame();
    if (frame == nullptr) {
        ImGui::TextUnformatted(u8c(u8"フレームがありません。"));
        return;
    }
    canvasRenderer_.setRenderer(renderer_);
    canvasRenderer_.setCanvasSize(project_.canvas.width, project_.canvas.height);
    const ImVec2 areaMin = ImGui::GetCursorScreenPos();
    const ImVec2 areaSize = ImGui::GetContentRegionAvail();
    fitCanvasToArea(areaSize);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const bool isColoringMode = currentMode_ == AppMode::Coloring;

    // Phase 1.5 Step 18:
    // 作画・彩色どちらでも、実際の紙に近い白いキャンバス背景にする。
    // パネルやアプリ全体のダークテーマは維持し、描画範囲だけ白くする。
    const ImU32 background = IM_COL32(255, 255, 255, 255);
    drawList->AddRectFilled(areaMin, ImVec2(areaMin.x + areaSize.x, areaMin.y + areaSize.y), background);
    const bool drawAssistOverlays = !isPlayingFrames_ || !playbackSkipAssistOverlays_;
    if (drawAssistOverlays) {
        const Cell* onionCell = activeCell();
        const Frame* previous = (onionCell != nullptr && activeFrameIndex_ > 0)
            ? onionCell->frameOrNull(activeFrameIndex_ - 1)
            : nullptr;
        const Frame* next = (onionCell != nullptr) ? onionCell->frameOrNull(activeFrameIndex_ + 1) : nullptr;
        if (onionPrevious_ && previous != nullptr) {
            drawOnionFrameDirect(*previous, true, 0.72f, canvasView_, areaMin, areaSize, drawList);
        }
        if (onionNext_ && next != nullptr) {
            drawOnionFrameDirect(*next, false, 0.72f, canvasView_, areaMin, areaSize, drawList);
        }
        drawLightTableOverlay(areaMin, areaSize, drawList);
    }
    const bool myPaintRealtimePreview = isMyPaintStrokeActive_;
    const Stroke* preview = (isDrawingStroke_
        && brushSettings_.tool == ui::ToolKind::Brush
        && !myPaintRealtimePreview) ? &currentStroke_ : nullptr;
        const CanvasDisplayMode canvasDisplayMode = isColoringMode ? CanvasDisplayMode::Coloring : CanvasDisplayMode::Drawing;

    const ui::CellDisplayMode cellDisplayMode = ui::currentCellDisplayMode();
    const int soloCellIndex = ui::currentSoloCellIndex();

    // Timesheet Rebuild Step 6.6:
    // タイムシートに1件以上入力がある場合だけ、T選択をキャンバス表示へ反映する。
    // ただし指パラ/通常タイムライン再生中は activeFrameIndex_ の再生表示を優先する。
    // activeFrameIndex_ は作画F編集対象として維持し、タイムシートT表示とは混ぜない。
    const int timesheetEntryCount = countTimesheetEntries(workingTimesheet_);
    const bool hasTimesheetEntries = timesheetEntryCount > 0;
    const bool useTimesheetPreview = hasTimesheetEntries && !isPlayingFrames_;
    const int timesheetTimelineFrame = useTimesheetPreview
        ? clampTimesheetFrame(workingTimesheet_, timesheetPanelState_.selectedTimelineFrame + 1)
        : activeFrameIndex_ + 1;

    auto drawCellFrame = [&](int cellIndex, const Cell& drawCell, const Frame& drawFrame, int drawFrameIndex) {
        const bool isActiveCell = cellIndex == activeCellIndex_;
        const bool isActiveEditingFrame = isActiveCell && drawFrameIndex == activeFrameIndex_;
        const int drawActiveLayer = isActiveEditingFrame ? activeLayerIndex_ : -1;
        const Stroke* drawPreview = isActiveEditingFrame ? preview : nullptr;
        const float drawOpacity = std::clamp(drawCell.opacity, 0.0f, 1.0f);
        if (drawOpacity <= 0.0f) {
            return;
        }

        canvasRenderer_.draw(drawFrame,
                             drawFrameIndex,
                             drawActiveLayer,
                             drawPreview,
                             drawOpacity,
                             canvasView_,
                             canvasDisplayMode,
                             areaMin,
                             areaSize,
                             drawList);
    };

    auto appendTimesheetSceneInput = [&](std::vector<TimesheetSceneCellInput>& inputs,
                                          int cellIndex,
                                          const Cell* cell,
                                          bool requireCellVisible) {
        if (cell == nullptr) {
            return;
        }
        inputs.push_back(TimesheetSceneCellInput{cell, cellIndex, requireCellVisible});
    };

    auto buildTimesheetSceneInputsForCurrentDisplay = [&]() {
        std::vector<TimesheetSceneCellInput> inputs;
        if (cellDisplayMode == ui::CellDisplayMode::ActiveOnly || project_.cells.empty()) {
            appendTimesheetSceneInput(inputs, activeCellIndex_, activeCell(), false);
            return inputs;
        }

        if (cellDisplayMode == ui::CellDisplayMode::SoloSelected) {
            const int safeSoloIndex = std::clamp(
                soloCellIndex >= 0 ? soloCellIndex : activeCellIndex_,
                0,
                static_cast<int>(project_.cells.size()) - 1);
            appendTimesheetSceneInput(inputs,
                                      safeSoloIndex,
                                      &project_.cells[static_cast<std::size_t>(safeSoloIndex)],
                                      false);
            return inputs;
        }

        inputs.reserve(project_.cells.size());
        for (int cellIndex = 0; cellIndex < static_cast<int>(project_.cells.size()); ++cellIndex) {
            appendTimesheetSceneInput(inputs,
                                      cellIndex,
                                      &project_.cells[static_cast<std::size_t>(cellIndex)],
                                      true);
        }
        return inputs;
    };

    int timesheetResolvedSceneCellCount = 0;
    if (useTimesheetPreview) {
        const ResolvedTimesheetSceneFrame resolvedScene = resolveTimesheetSceneFrame(
            workingTimesheet_,
            buildTimesheetSceneInputsForCurrentDisplay(),
            timesheetTimelineFrame);
        timesheetResolvedSceneCellCount = static_cast<int>(resolvedScene.cells.size());
        for (const ResolvedTimesheetSceneCell& resolvedCell : resolvedScene.cells) {
            if (resolvedCell.cell == nullptr || resolvedCell.drawingFrameIndex < 0) {
                continue;
            }
            const Frame& drawFrame = resolvedCell.cell->frames[static_cast<std::size_t>(resolvedCell.drawingFrameIndex)];
            drawCellFrame(resolvedCell.cellIndex, *resolvedCell.cell, drawFrame, resolvedCell.drawingFrameIndex);
        }
    } else if (cellDisplayMode == ui::CellDisplayMode::ActiveOnly || project_.cells.empty()) {
        drawCellFrame(activeCellIndex_, *activeCell(), *frame, activeFrameIndex_);
    } else if (cellDisplayMode == ui::CellDisplayMode::SoloSelected) {
        const int safeSoloIndex = std::clamp(
            soloCellIndex >= 0 ? soloCellIndex : activeCellIndex_,
            0,
            static_cast<int>(project_.cells.size()) - 1);
        const Cell& soloCell = project_.cells[static_cast<std::size_t>(safeSoloIndex)];
        if (!soloCell.frames.empty()) {
            const int soloFrameIndex = std::clamp(activeFrameIndex_, 0, static_cast<int>(soloCell.frames.size()) - 1);
            const Frame& soloFrame = soloCell.frames[static_cast<std::size_t>(soloFrameIndex)];
            drawCellFrame(safeSoloIndex, soloCell, soloFrame, soloFrameIndex);
        }
    } else {
        for (int cellIndex = 0; cellIndex < static_cast<int>(project_.cells.size()); ++cellIndex) {
            const Cell& drawCell = project_.cells[static_cast<std::size_t>(cellIndex)];
            if (!drawCell.visible || drawCell.opacity <= 0.0f || drawCell.frames.empty()) {
                continue;
            }
            const int drawFrameIndex = std::clamp(activeFrameIndex_, 0, static_cast<int>(drawCell.frames.size()) - 1);
            const Frame& drawFrame = drawCell.frames[static_cast<std::size_t>(drawFrameIndex)];
            drawCellFrame(cellIndex, drawCell, drawFrame, drawFrameIndex);
        }
    }

    bool allowCanvasInput = true;
    if (useTimesheetPreview) {
        if (const Cell* editingCell = activeCell(); editingCell != nullptr) {
            const ResolvedTimesheetCell activeResolved =
                resolveTimesheetCell(workingTimesheet_, editingCell->id, timesheetTimelineFrame);
            allowCanvasInput = activeResolved.visible && activeResolved.drawingFrameNumber == activeFrameIndex_ + 1;
        }
    }

    if (hasTimesheetEntries) {
        // Timesheet Rebuild Step 7.2:
        // キャンバス左上の状態表示は、前段の大きい説明ボックスではなく小型バッジに戻す。
        // 「表示セルN」は残しつつ、作画画面を邪魔しないサイズにする。
        const std::string previewText = std::string(u8c(u8"TS ")) +
            (useTimesheetPreview ? std::string("ON") : std::string("OFF")) +
            std::string(u8c(u8" T")) +
            std::to_string(timesheetTimelineFrame) +
            std::string(u8c(u8" 編集F")) +
            std::to_string(activeFrameIndex_ + 1) +
            std::string(u8c(u8" 表示セルN=")) +
            std::to_string(timesheetResolvedSceneCellCount) +
            std::string(u8c(u8" entries=")) +
            std::to_string(timesheetEntryCount);
        const char* warningText = u8c(u8"表示F≠編集F: 作画入力停止 / ズーム・移動可");
        const ImVec2 textSize = ImGui::CalcTextSize(previewText.c_str());
        const ImVec2 warningSize = !allowCanvasInput ? ImGui::CalcTextSize(warningText) : ImVec2(0.0f, 0.0f);
        const float overlayWidth = std::min(areaSize.x - 16.0f,
                                           std::max(textSize.x, warningSize.x) + 14.0f);
        const float overlayHeight = !allowCanvasInput ? 42.0f : 24.0f;
        drawList->AddRectFilled(
            ImVec2(areaMin.x + 8.0f, areaMin.y + 8.0f),
            ImVec2(areaMin.x + 8.0f + overlayWidth, areaMin.y + 8.0f + overlayHeight),
            IM_COL32(255, 255, 255, 220),
            4.0f);
        drawList->AddRect(
            ImVec2(areaMin.x + 8.0f, areaMin.y + 8.0f),
            ImVec2(areaMin.x + 8.0f + overlayWidth, areaMin.y + 8.0f + overlayHeight),
            IM_COL32(80, 80, 80, 180),
            4.0f);
        drawList->AddText(
            ImVec2(areaMin.x + 14.0f, areaMin.y + 13.0f),
            IM_COL32(25, 25, 25, 255),
            previewText.c_str());
        if (!allowCanvasInput) {
            drawList->AddText(
                ImVec2(areaMin.x + 14.0f, areaMin.y + 31.0f),
                IM_COL32(180, 40, 40, 255),
                warningText);
        }
    }
    warmPlaybackFrameCache();
    if (isDrawingStroke_ && brushSettings_.tool == ui::ToolKind::Eraser && !currentStroke_.points.empty()) {
        drawLightweightEraserPreview(currentStroke_, canvasView_, areaMin, areaSize, drawList);
    }
    const bool eraserCursor = isDrawingStroke_
        && brushSettings_.tool == ui::ToolKind::Eraser
        && !currentStroke_.points.empty();
    if (eraserCursor) {
        const StrokePoint& point = currentStroke_.points.back();
        const ImVec2 screenPoint = canvasView_.canvasToScreen(point.x, point.y, areaMin, areaSize);
        const float radius = std::max(2.0f, currentStroke_.radiusPx * std::clamp(canvasView_.zoom, 0.05f, 32.0f));
        drawList->AddCircle(screenPoint, radius, IM_COL32(255, 80, 70, 210), 32, 2.0f);
    }
    // allowCanvasInput=false は作画入力だけを止める。ズーム/中ボタン移動は常に使える。
    handleCanvasInput(areaMin, areaSize, allowCanvasInput);
    ImGui::Dummy(areaSize);
}
void App::fitCanvasToArea(ImVec2 areaSize)
{
    if (canvasViewInitialized_ || areaSize.x <= 1.0f || areaSize.y <= 1.0f) {
        return;
    }
    const float zoomX = areaSize.x / static_cast<float>(std::max(1, project_.canvas.width));
    const float zoomY = areaSize.y / static_cast<float>(std::max(1, project_.canvas.height));
    canvasView_.zoom = std::clamp(std::min(zoomX, zoomY) * 0.9f, 0.05f, 4.0f);
    canvasView_.panX = (areaSize.x - static_cast<float>(project_.canvas.width) * canvasView_.zoom) * 0.5f;
    canvasView_.panY = (areaSize.y - static_cast<float>(project_.canvas.height) * canvasView_.zoom) * 0.5f;
    canvasViewInitialized_ = true;
}
void App::finishStroke()
{
    if (!isDrawingStroke_) {
        return;
    }

    const bool useMyPaintRealtime = isMyPaintStrokeActive_;

    if (!currentStroke_.points.empty()) {
        pushUndoSnapshot();
        Layer* layer = activeLayer();
        if (layer != nullptr) {
            if (brushSettings_.tool == ui::ToolKind::Brush) {
                if (useMyPaintRealtime) {
                    // MyPaint逐次処理: ピクセルはaddPoint()で書き込み済み。
                    // bakeStrokeは呼ばない。点列の保存とendStroke()のみ。
                    myPaintEngine_.endStroke();
                    isMyPaintStrokeActive_ = false;
                    layer->strokes.push_back(currentStroke_);

                    // markAllDirtyは不要。ここでは直接描画済みCanvasBitmapとProject点列のrevisionだけ同期する。
                    canvasRenderer_.markActiveBitmapClean(activeLayerIndex_, *layer);
                    lastMessage_ = "MyPaint stroke committed";
                } else {
                    // Simple互換: 確定時にbakeStrokeで焼き込む。
                    layer->strokes.push_back(currentStroke_);
                    canvasRenderer_.bakeStrokeOnLayer(activeLayerIndex_, currentStroke_, currentStroke_.opacity);
                    canvasRenderer_.markAllDirty();
                    lastMessage_ = "stroke committed to active frame";
                }
            } else {
                removeIntersectingStrokes(currentStroke_);
                canvasRenderer_.markAllDirty();
            }
        }
    } else if (useMyPaintRealtime) {
        myPaintEngine_.endStroke();
    }

    currentStroke_ = Stroke{};
    isDrawingStroke_ = false;
    isMyPaintStrokeActive_ = false;
}
void App::cancelStroke()
{
    const bool useMyPaintRealtime = isMyPaintStrokeActive_;
    if (useMyPaintRealtime) {
        myPaintEngine_.endStroke();
        // 未確定の直接描画ピクセルを捨てるため、保存済みストロークから再構築する。
        canvasRenderer_.markAllDirty();
    }
    currentStroke_ = Stroke{};
    isDrawingStroke_ = false;
    isMyPaintStrokeActive_ = false;
}
void App::removeIntersectingStrokes(const Stroke& eraserStroke)
{
    Layer* layer = activeLayer();
    if (layer == nullptr || eraserStroke.points.empty()) {
        return;
    }
    const float radius = std::max(1.0f, eraserStroke.radiusPx);
    std::vector<Stroke> rewrittenStrokes;
    rewrittenStrokes.reserve(layer->strokes.size());
    bool changed = false;
    const std::size_t beforeCount = layer->strokes.size();
    for (const Stroke& stroke : layer->strokes) {
        std::vector<Stroke> parts = splitStrokeByEraser(stroke, eraserStroke, radius, changed);
        rewrittenStrokes.insert(rewrittenStrokes.end(),
                                std::make_move_iterator(parts.begin()),
                                std::make_move_iterator(parts.end()));
    }
    if (changed) {
        layer->strokes = std::move(rewrittenStrokes);
        lastMessage_ = "eraser applied: partial stroke split v22";
    } else {
        lastMessage_ = "eraser applied: no hit, radius=" + std::to_string(radius) +
                       ", strokes=" + std::to_string(beforeCount);
    }
}
} // namespace perapera

