// src/ui/DrawingCanvasPanel.cpp
//
// DrawingCanvasPanelの実装です。
//
// Phase 3Lでは、フレーム名・レイヤー名変更に対応します。
// frames_ がAnimationFrameの配列を持ち、
// 各AnimationFrameがDrawingLayerの配列を持ちます。
//
// できること:
// - フレームを追加する
// - フレームを複製する
// - フレームを削除する
// - フレームごとに別々のレイヤーと線を持つ
// - 前後フレームをオニオンスキン表示する
// - 現在フレームをPNG保存する
// - FPSと保持コマ数に従って再生プレビューする
// - 全フレームをdurationFrames込みでPNG連番保存する
// - 作業内容をプロジェクトファイルとして保存/読み込みする
// - 描画、フレーム操作、レイヤー操作をUndo/Redoする
// - 消しゴムで選択中レイヤー上の線を消す
// - タイムラインでフレーム・保持コマ数・再生位置を確認する

#include "ui/DrawingCanvasPanel.h"

#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>

namespace perapera
{
    namespace
    {
        constexpr ImU32 PreviewBackgroundColor = IM_COL32(18, 20, 24, 255);
        constexpr ImU32 CanvasFillColor = IM_COL32(44, 48, 56, 255);
        constexpr ImU32 CanvasBorderColor = IM_COL32(150, 170, 210, 255);
        constexpr ImU32 GridLineColor = IM_COL32(80, 86, 96, 120);
        constexpr ImU32 RenderFrameColor = IM_COL32(255, 220, 80, 255);
        constexpr ImU32 WarningColor = IM_COL32(255, 120, 80, 255);

        std::string makeDefaultFrameName(int frameNumber)
        {
            return "Frame " + std::to_string(frameNumber);
        }

        std::string makeDefaultLayerName(int layerNumber)
        {
            return "Layer " + std::to_string(layerNumber);
        }

        DrawingLayer makeDefaultLayer(const std::string& name)
        {
            DrawingLayer layer;
            layer.name = name;
            layer.visible = true;
            layer.opacity = 1.0f;
            return layer;
        }

        AnimationFrame makeDefaultFrame(int frameNumber)
        {
            AnimationFrame frame;
            frame.name = makeDefaultFrameName(frameNumber);
            frame.durationFrames = 1;
            frame.layers.push_back(makeDefaultLayer("Layer 1"));
            return frame;
        }

        int calculatePngSequenceImageCount(const std::vector<AnimationFrame>& frames)
        {
            int imageCount = 0;

            for (const AnimationFrame& frame : frames)
            {
                imageCount += std::max(1, frame.durationFrames);
            }

            return imageCount;
        }

        void copyStringToInputBuffer(
            const std::string& source,
            char* buffer,
            std::size_t bufferSize
        )
        {
            if (buffer == nullptr || bufferSize == 0)
            {
                return;
            }

            const std::size_t copySize = std::min(
                source.size(),
                bufferSize - 1
            );

            std::copy_n(source.c_str(), copySize, buffer);
            buffer[copySize] = '\0';
        }

        std::string normalizeEditableName(
            const char* rawName,
            const std::string& fallbackName
        )
        {
            std::string name = rawName != nullptr ? std::string(rawName) : std::string();

            // 改行など、保存形式やUIを崩しやすい制御文字は空白に置き換える。
            for (char& character : name)
            {
                if (character == '\r' || character == '\n' || character == '\t')
                {
                    character = ' ';
                }
            }

            const bool hasVisibleCharacter = std::any_of(
                name.begin(),
                name.end(),
                [](unsigned char character)
                {
                    return !std::isspace(character);
                }
            );

            if (!hasVisibleCharacter)
            {
                return fallbackName;
            }

            return name;
        }

        std::filesystem::path findProjectRootFromCurrentPath()
        {
            std::error_code currentPathError;
            std::filesystem::path currentPath =
                std::filesystem::current_path(currentPathError);

            if (currentPathError)
            {
                return std::filesystem::path(".");
            }

            std::filesystem::path candidate = currentPath;

            while (!candidate.empty())
            {
                std::error_code existsError;

                const bool hasCMakeLists =
                    std::filesystem::exists(candidate / "CMakeLists.txt", existsError);

                existsError.clear();

                const bool hasSrcDirectory =
                    std::filesystem::exists(candidate / "src", existsError);

                if (hasCMakeLists && hasSrcDirectory)
                {
                    return candidate;
                }

                const std::filesystem::path parent = candidate.parent_path();

                if (parent == candidate)
                {
                    break;
                }

                candidate = parent;
            }

            return currentPath;
        }

        std::filesystem::path makeProjectFilePathFromProjectRoot(
            const std::string& projectFilePath
        )
        {
            const std::filesystem::path path(projectFilePath);

            if (path.is_absolute())
            {
                return path;
            }

            return findProjectRootFromCurrentPath() / path;
        }


        bool readExpectedKeyword(
            std::istream& input,
            const std::string& expectedKeyword,
            std::string& errorMessage
        )
        {
            std::string actualKeyword;

            if (!(input >> actualKeyword))
            {
                errorMessage = "プロジェクト読み込み失敗: "
                    + expectedKeyword
                    + " を読む前にファイル終端に到達しました。";
                return false;
            }

            if (actualKeyword != expectedKeyword)
            {
                errorMessage = "プロジェクト読み込み失敗: "
                    + expectedKeyword
                    + " が必要な場所で "
                    + actualKeyword
                    + " を読みました。";
                return false;
            }

            return true;
        }

        void writeProjectFileToStream(
            std::ostream& output,
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat,
            const Brush& brush,
            const std::vector<AnimationFrame>& frames
        )
        {
            output << "PERAPERA_ANIME_MAKER_PROJECT_V1\n";

            output
                << "WORK_CANVAS "
                << workCanvas.widthPx
                << ' '
                << workCanvas.heightPx
                << "\n";

            output
                << "RENDER_FORMAT "
                << renderFormat.outputWidthPx
                << ' '
                << renderFormat.outputHeightPx
                << ' '
                << renderFormat.framesPerSecond
                << ' '
                << renderFormat.pixelAspectRatio
                << "\n";

            output
                << "BRUSH "
                << brush.radiusPx
                << ' '
                << brush.color.r
                << ' '
                << brush.color.g
                << ' '
                << brush.color.b
                << ' '
                << brush.color.a
                << "\n";

            output << "FRAMES " << frames.size() << "\n";

            for (const AnimationFrame& frame : frames)
            {
                output
                    << "FRAME "
                    << std::quoted(frame.name)
                    << ' '
                    << frame.durationFrames
                    << ' '
                    << frame.layers.size()
                    << "\n";

                for (const DrawingLayer& layer : frame.layers)
                {
                    output
                        << "LAYER "
                        << std::quoted(layer.name)
                        << ' '
                        << (layer.visible ? 1 : 0)
                        << ' '
                        << layer.opacity
                        << ' '
                        << layer.strokes.size()
                        << "\n";

                    for (const Stroke& stroke : layer.strokes)
                    {
                        output
                            << "STROKE "
                            << stroke.radiusPx
                            << ' '
                            << stroke.color.r
                            << ' '
                            << stroke.color.g
                            << ' '
                            << stroke.color.b
                            << ' '
                            << stroke.color.a
                            << ' '
                            << stroke.points.size()
                            << "\n";

                        for (const CanvasPoint& point : stroke.points)
                        {
                            output
                                << "POINT "
                                << point.x
                                << ' '
                                << point.y
                                << "\n";
                        }

                        output << "END_STROKE\n";
                    }

                    output << "END_LAYER\n";
                }

                output << "END_FRAME\n";
            }

            output << "END_PROJECT\n";
        }

        bool readProjectFileFromStream(
            std::istream& input,
            WorkCanvas& workCanvas,
            RenderFormat& renderFormat,
            Brush& brush,
            std::vector<AnimationFrame>& frames,
            std::string& errorMessage
        )
        {
            if (!readExpectedKeyword(input, "PERAPERA_ANIME_MAKER_PROJECT_V1", errorMessage))
            {
                return false;
            }

            int loadedCanvasWidth = 0;
            int loadedCanvasHeight = 0;

            if (!readExpectedKeyword(input, "WORK_CANVAS", errorMessage))
            {
                return false;
            }

            if (!(input >> loadedCanvasWidth >> loadedCanvasHeight))
            {
                errorMessage = "プロジェクト読み込み失敗: 作画キャンバス設定を読めません。";
                return false;
            }

            int loadedOutputWidth = 0;
            int loadedOutputHeight = 0;
            int loadedFps = 0;
            float loadedPixelAspectRatio = 1.0f;

            if (!readExpectedKeyword(input, "RENDER_FORMAT", errorMessage))
            {
                return false;
            }

            if (!(input >> loadedOutputWidth >> loadedOutputHeight >> loadedFps >> loadedPixelAspectRatio))
            {
                errorMessage = "プロジェクト読み込み失敗: 撮影フレーム設定を読めません。";
                return false;
            }

            Brush loadedBrush;

            if (!readExpectedKeyword(input, "BRUSH", errorMessage))
            {
                return false;
            }

            if (!(input
                >> loadedBrush.radiusPx
                >> loadedBrush.color.r
                >> loadedBrush.color.g
                >> loadedBrush.color.b
                >> loadedBrush.color.a))
            {
                errorMessage = "プロジェクト読み込み失敗: ブラシ設定を読めません。";
                return false;
            }

            int frameCount = 0;

            if (!readExpectedKeyword(input, "FRAMES", errorMessage))
            {
                return false;
            }

            if (!(input >> frameCount) || frameCount <= 0)
            {
                errorMessage = "プロジェクト読み込み失敗: フレーム数が不正です。";
                return false;
            }

            std::vector<AnimationFrame> loadedFrames;
            loadedFrames.reserve(static_cast<std::size_t>(frameCount));

            for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
            {
                if (!readExpectedKeyword(input, "FRAME", errorMessage))
                {
                    return false;
                }

                AnimationFrame frame;
                int layerCount = 0;

                if (!(input >> std::quoted(frame.name) >> frame.durationFrames >> layerCount)
                    || layerCount <= 0)
                {
                    errorMessage = "プロジェクト読み込み失敗: フレーム情報が不正です。";
                    return false;
                }

                frame.durationFrames = std::clamp(frame.durationFrames, 1, 240);
                frame.layers.clear();
                frame.layers.reserve(static_cast<std::size_t>(layerCount));

                for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
                {
                    if (!readExpectedKeyword(input, "LAYER", errorMessage))
                    {
                        return false;
                    }

                    DrawingLayer layer;
                    int visibleValue = 1;
                    int strokeCount = 0;

                    if (!(input
                        >> std::quoted(layer.name)
                        >> visibleValue
                        >> layer.opacity
                        >> strokeCount)
                        || strokeCount < 0)
                    {
                        errorMessage = "プロジェクト読み込み失敗: レイヤー情報が不正です。";
                        return false;
                    }

                    layer.visible = (visibleValue != 0);
                    layer.opacity = std::clamp(layer.opacity, 0.0f, 1.0f);
                    layer.strokes.clear();
                    layer.strokes.reserve(static_cast<std::size_t>(strokeCount));

                    for (int strokeIndex = 0; strokeIndex < strokeCount; ++strokeIndex)
                    {
                        if (!readExpectedKeyword(input, "STROKE", errorMessage))
                        {
                            return false;
                        }

                        Stroke stroke;
                        int pointCount = 0;

                        if (!(input
                            >> stroke.radiusPx
                            >> stroke.color.r
                            >> stroke.color.g
                            >> stroke.color.b
                            >> stroke.color.a
                            >> pointCount)
                            || pointCount < 0)
                        {
                            errorMessage = "プロジェクト読み込み失敗: ストローク情報が不正です。";
                            return false;
                        }

                        stroke.radiusPx = std::clamp(stroke.radiusPx, 0.1f, 512.0f);
                        stroke.color.r = std::clamp(stroke.color.r, 0.0f, 1.0f);
                        stroke.color.g = std::clamp(stroke.color.g, 0.0f, 1.0f);
                        stroke.color.b = std::clamp(stroke.color.b, 0.0f, 1.0f);
                        stroke.color.a = std::clamp(stroke.color.a, 0.0f, 1.0f);
                        stroke.points.reserve(static_cast<std::size_t>(pointCount));

                        for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
                        {
                            if (!readExpectedKeyword(input, "POINT", errorMessage))
                            {
                                return false;
                            }

                            CanvasPoint point;

                            if (!(input >> point.x >> point.y))
                            {
                                errorMessage = "プロジェクト読み込み失敗: 点情報が不正です。";
                                return false;
                            }

                            stroke.points.push_back(point);
                        }

                        if (!readExpectedKeyword(input, "END_STROKE", errorMessage))
                        {
                            return false;
                        }

                        layer.strokes.push_back(stroke);
                    }

                    if (!readExpectedKeyword(input, "END_LAYER", errorMessage))
                    {
                        return false;
                    }

                    frame.layers.push_back(layer);
                }

                if (!readExpectedKeyword(input, "END_FRAME", errorMessage))
                {
                    return false;
                }

                loadedFrames.push_back(frame);
            }

            if (!readExpectedKeyword(input, "END_PROJECT", errorMessage))
            {
                return false;
            }

            workCanvas.widthPx = std::clamp(loadedCanvasWidth, 16, 32768);
            workCanvas.heightPx = std::clamp(loadedCanvasHeight, 16, 32768);

            renderFormat.outputWidthPx = std::clamp(loadedOutputWidth, 16, 16384);
            renderFormat.outputHeightPx = std::clamp(loadedOutputHeight, 16, 16384);
            renderFormat.framesPerSecond = std::clamp(loadedFps, 1, 240);
            renderFormat.pixelAspectRatio = std::clamp(loadedPixelAspectRatio, 0.1f, 10.0f);

            loadedBrush.radiusPx = std::clamp(loadedBrush.radiusPx, 0.1f, 512.0f);
            loadedBrush.color.r = std::clamp(loadedBrush.color.r, 0.0f, 1.0f);
            loadedBrush.color.g = std::clamp(loadedBrush.color.g, 0.0f, 1.0f);
            loadedBrush.color.b = std::clamp(loadedBrush.color.b, 0.0f, 1.0f);
            loadedBrush.color.a = std::clamp(loadedBrush.color.a, 0.0f, 1.0f);

            brush = loadedBrush;
            frames = loadedFrames;

            return true;
        }

        float distanceSquared(CanvasPoint a, CanvasPoint b)
        {
            const float dx = a.x - b.x;
            const float dy = a.y - b.y;
            return dx * dx + dy * dy;
        }

        float distancePointToSegmentSquared(
            CanvasPoint point,
            CanvasPoint segmentStart,
            CanvasPoint segmentEnd
        )
        {
            const float segmentX = segmentEnd.x - segmentStart.x;
            const float segmentY = segmentEnd.y - segmentStart.y;
            const float segmentLengthSquared = segmentX * segmentX + segmentY * segmentY;

            if (segmentLengthSquared <= 0.0001f)
            {
                return distanceSquared(point, segmentStart);
            }

            const float pointX = point.x - segmentStart.x;
            const float pointY = point.y - segmentStart.y;

            const float t = std::clamp(
                (pointX * segmentX + pointY * segmentY) / segmentLengthSquared,
                0.0f,
                1.0f
            );

            const CanvasPoint closestPoint{
                segmentStart.x + segmentX * t,
                segmentStart.y + segmentY * t
            };

            return distanceSquared(point, closestPoint);
        }

        void appendStrokePartIfNotEmpty(
            std::vector<Stroke>& outputStrokes,
            const Stroke& originalStroke,
            std::vector<CanvasPoint>& points
        )
        {
            if (points.empty())
            {
                return;
            }

            Stroke strokePart = originalStroke;
            strokePart.points = points;
            outputStrokes.push_back(strokePart);
            points.clear();
        }

        bool eraseLayerWithPath(
            DrawingLayer& layer,
            CanvasPoint eraserPathStart,
            CanvasPoint eraserPathEnd,
            bool hasPathStart,
            float eraserRadiusPx
        )
        {
            bool changed = false;
            std::vector<Stroke> updatedStrokes;
            updatedStrokes.reserve(layer.strokes.size());

            for (const Stroke& stroke : layer.strokes)
            {
                std::vector<CanvasPoint> keptPointsForCurrentPart;
                keptPointsForCurrentPart.reserve(stroke.points.size());

                bool strokeChanged = false;

                const float eraseDistance = std::max(1.0f, eraserRadiusPx + stroke.radiusPx);
                const float eraseDistanceSquared = eraseDistance * eraseDistance;

                for (const CanvasPoint& point : stroke.points)
                {
                    bool shouldErasePoint =
                        distanceSquared(point, eraserPathEnd) <= eraseDistanceSquared;

                    if (hasPathStart)
                    {
                        shouldErasePoint = shouldErasePoint
                            || distancePointToSegmentSquared(
                                point,
                                eraserPathStart,
                                eraserPathEnd
                            ) <= eraseDistanceSquared;
                    }

                    if (shouldErasePoint)
                    {
                        strokeChanged = true;
                        appendStrokePartIfNotEmpty(
                            updatedStrokes,
                            stroke,
                            keptPointsForCurrentPart
                        );
                    }
                    else
                    {
                        keptPointsForCurrentPart.push_back(point);
                    }
                }

                appendStrokePartIfNotEmpty(
                    updatedStrokes,
                    stroke,
                    keptPointsForCurrentPart
                );

                if (strokeChanged)
                {
                    changed = true;
                }
            }

            if (changed)
            {
                layer.strokes = std::move(updatedStrokes);
            }

            return changed;
        }

        ImU32 toImGuiColor(ColorRgba color, float layerOpacity)
        {
            const float finalAlpha = std::clamp(color.a * layerOpacity, 0.0f, 1.0f);

            const int r = static_cast<int>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
            const int g = static_cast<int>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
            const int b = static_cast<int>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
            const int a = static_cast<int>(finalAlpha * 255.0f);

            return IM_COL32(r, g, b, a);
        }

        CanvasPoint screenToCanvasPoint(
            ImVec2 screenPoint,
            ImVec2 canvasMin,
            float scale
        )
        {
            return CanvasPoint{
                (screenPoint.x - canvasMin.x) / scale,
                (screenPoint.y - canvasMin.y) / scale
            };
        }

        ImVec2 canvasToScreenPoint(
            CanvasPoint canvasPoint,
            ImVec2 canvasMin,
            float scale
        )
        {
            return ImVec2(
                canvasMin.x + canvasPoint.x * scale,
                canvasMin.y + canvasPoint.y * scale
            );
        }

        bool isInsideRect(ImVec2 point, ImVec2 min, ImVec2 max)
        {
            return point.x >= min.x
                && point.y >= min.y
                && point.x <= max.x
                && point.y <= max.y;
        }

        void drawStroke(
            ImDrawList* drawList,
            const Stroke& stroke,
            ImVec2 canvasMin,
            float scale,
            float layerOpacity
        )
        {
            if (!stroke.hasDrawablePoints())
            {
                return;
            }

            const ImU32 color = toImGuiColor(stroke.color, layerOpacity);
            const float thickness = std::max(1.0f, stroke.radiusPx * 2.0f * scale);

            if (stroke.points.size() == 1)
            {
                const ImVec2 center = canvasToScreenPoint(stroke.points.front(), canvasMin, scale);
                drawList->AddCircleFilled(center, thickness * 0.5f, color);
                return;
            }

            for (std::size_t i = 1; i < stroke.points.size(); ++i)
            {
                const ImVec2 a = canvasToScreenPoint(stroke.points[i - 1], canvasMin, scale);
                const ImVec2 b = canvasToScreenPoint(stroke.points[i], canvasMin, scale);

                drawList->AddLine(a, b, color, thickness);
            }
        }

        ImU32 toOnionSkinColor(
            bool isPreviousFrame,
            float onionOpacity,
            float layerOpacity,
            float strokeAlpha
        )
        {
            const float finalAlpha = std::clamp(
                onionOpacity * layerOpacity * strokeAlpha,
                0.0f,
                1.0f
            );

            const int alphaByte = static_cast<int>(finalAlpha * 255.0f);

            if (isPreviousFrame)
            {
                return IM_COL32(255, 110, 80, alphaByte);
            }

            return IM_COL32(80, 180, 255, alphaByte);
        }

        void drawStrokeWithFixedColor(
            ImDrawList* drawList,
            const Stroke& stroke,
            ImVec2 canvasMin,
            float scale,
            ImU32 color
        )
        {
            if (!stroke.hasDrawablePoints())
            {
                return;
            }

            const float thickness = std::max(1.0f, stroke.radiusPx * 2.0f * scale);

            if (stroke.points.size() == 1)
            {
                const ImVec2 center = canvasToScreenPoint(stroke.points.front(), canvasMin, scale);
                drawList->AddCircleFilled(center, thickness * 0.5f, color);
                return;
            }

            for (std::size_t i = 1; i < stroke.points.size(); ++i)
            {
                const ImVec2 a = canvasToScreenPoint(stroke.points[i - 1], canvasMin, scale);
                const ImVec2 b = canvasToScreenPoint(stroke.points[i], canvasMin, scale);

                drawList->AddLine(a, b, color, thickness);
            }
        }

        void drawOnionSkinFrame(
            ImDrawList* drawList,
            const AnimationFrame& frame,
            ImVec2 canvasMin,
            float scale,
            bool isPreviousFrame,
            float onionOpacity,
            bool visibleLayersOnly
        )
        {
            for (const DrawingLayer& layer : frame.layers)
            {
                if (visibleLayersOnly && !layer.visible)
                {
                    continue;
                }

                const float layerOpacity = std::clamp(layer.opacity, 0.0f, 1.0f);

                for (const Stroke& stroke : layer.strokes)
                {
                    const ImU32 onionColor = toOnionSkinColor(
                        isPreviousFrame,
                        onionOpacity,
                        layerOpacity,
                        stroke.color.a
                    );

                    drawStrokeWithFixedColor(
                        drawList,
                        stroke,
                        canvasMin,
                        scale,
                        onionColor
                    );
                }
            }
        }
    }

    DrawingCanvasPanel::DrawingCanvasPanel()
    {
        frames_.push_back(makeDefaultFrame(1));

        activeFrameIndex_ = 0;
        activeLayerIndex_ = 0;
        nextFrameNumber_ = 2;
        nextLayerNumber_ = 2;
    }

    void DrawingCanvasPanel::clampActiveFrameIndex()
    {
        if (frames_.empty())
        {
            activeFrameIndex_ = -1;
            return;
        }

        activeFrameIndex_ = std::clamp(
            activeFrameIndex_,
            0,
            static_cast<int>(frames_.size()) - 1
        );
    }

    void DrawingCanvasPanel::clampActiveLayerIndex()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->layers.empty())
        {
            activeLayerIndex_ = -1;
            return;
        }

        activeLayerIndex_ = std::clamp(
            activeLayerIndex_,
            0,
            static_cast<int>(frame->layers.size()) - 1
        );
    }

    AnimationFrame* DrawingCanvasPanel::activeFrame()
    {
        if (frames_.empty())
        {
            return nullptr;
        }

        clampActiveFrameIndex();

        if (activeFrameIndex_ < 0 || activeFrameIndex_ >= static_cast<int>(frames_.size()))
        {
            return nullptr;
        }

        return &frames_[static_cast<std::size_t>(activeFrameIndex_)];
    }

    const AnimationFrame* DrawingCanvasPanel::activeFrame() const
    {
        if (activeFrameIndex_ < 0 || activeFrameIndex_ >= static_cast<int>(frames_.size()))
        {
            return nullptr;
        }

        return &frames_[static_cast<std::size_t>(activeFrameIndex_)];
    }

    DrawingLayer* DrawingCanvasPanel::activeLayer()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->layers.empty())
        {
            return nullptr;
        }

        clampActiveLayerIndex();

        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(frame->layers.size()))
        {
            return nullptr;
        }

        return &frame->layers[static_cast<std::size_t>(activeLayerIndex_)];
    }

    const DrawingLayer* DrawingCanvasPanel::activeLayer() const
    {
        const AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->layers.empty())
        {
            return nullptr;
        }

        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(frame->layers.size()))
        {
            return nullptr;
        }

        return &frame->layers[static_cast<std::size_t>(activeLayerIndex_)];
    }

    bool DrawingCanvasPanel::eraseActiveLayerAt(CanvasPoint eraserCanvasPoint)
    {
        DrawingLayer* layer = activeLayer();

        if (layer == nullptr || !layer->visible)
        {
            return false;
        }

        return eraseLayerWithPath(
            *layer,
            eraserCanvasPoint,
            eraserCanvasPoint,
            false,
            eraserRadiusPx_
        );
    }

    bool DrawingCanvasPanel::eraseActiveLayerAlongPath(
        CanvasPoint previousCanvasPoint,
        CanvasPoint currentCanvasPoint
    )
    {
        DrawingLayer* layer = activeLayer();

        if (layer == nullptr || !layer->visible)
        {
            return false;
        }

        return eraseLayerWithPath(
            *layer,
            previousCanvasPoint,
            currentCanvasPoint,
            true,
            eraserRadiusPx_
        );
    }


    DrawingCanvasPanel::EditorHistorySnapshot DrawingCanvasPanel::makeHistorySnapshot() const
    {
        EditorHistorySnapshot snapshot;
        snapshot.frames = frames_;
        snapshot.activeFrameIndex = activeFrameIndex_;
        snapshot.activeLayerIndex = activeLayerIndex_;
        snapshot.nextFrameNumber = nextFrameNumber_;
        snapshot.nextLayerNumber = nextLayerNumber_;
        return snapshot;
    }

    void DrawingCanvasPanel::restoreHistorySnapshot(const EditorHistorySnapshot& snapshot)
    {
        stopPlayback();

        frames_ = snapshot.frames;

        if (frames_.empty())
        {
            frames_.push_back(makeDefaultFrame(1));
        }

        activeFrameIndex_ = snapshot.activeFrameIndex;
        activeLayerIndex_ = snapshot.activeLayerIndex;
        nextFrameNumber_ = std::max(2, snapshot.nextFrameNumber);
        nextLayerNumber_ = std::max(2, snapshot.nextLayerNumber);

        clampActiveFrameIndex();
        clampActiveLayerIndex();

        currentStroke_.points.clear();
        isDrawing_ = false;
        isErasing_ = false;
        hasPreviousEraserPoint_ = false;
    }

    void DrawingCanvasPanel::pushUndoSnapshot(const std::string& actionName)
    {
        stopPlayback();

        undoStack_.push_back(makeHistorySnapshot());

        if (undoStack_.size() > static_cast<std::size_t>(MaxUndoHistoryCount))
        {
            undoStack_.erase(undoStack_.begin());
        }

        redoStack_.clear();

        if (!actionName.empty())
        {
            lastUndoRedoMessage_ = "Undo記録: " + actionName;
        }
    }

    void DrawingCanvasPanel::clearHistory()
    {
        undoStack_.clear();
        redoStack_.clear();
        lastUndoRedoMessage_.clear();
    }

    void DrawingCanvasPanel::undoLastAction()
    {
        if (undoStack_.empty())
        {
            lastUndoRedoMessage_ = "Undoできる操作がありません。";
            return;
        }

        redoStack_.push_back(makeHistorySnapshot());

        const EditorHistorySnapshot snapshot = undoStack_.back();
        undoStack_.pop_back();
        restoreHistorySnapshot(snapshot);

        lastUndoRedoMessage_ = "元に戻しました。";
    }

    void DrawingCanvasPanel::redoLastAction()
    {
        if (redoStack_.empty())
        {
            lastUndoRedoMessage_ = "Redoできる操作がありません。";
            return;
        }

        undoStack_.push_back(makeHistorySnapshot());

        if (undoStack_.size() > static_cast<std::size_t>(MaxUndoHistoryCount))
        {
            undoStack_.erase(undoStack_.begin());
        }

        const EditorHistorySnapshot snapshot = redoStack_.back();
        redoStack_.pop_back();
        restoreHistorySnapshot(snapshot);

        lastUndoRedoMessage_ = "やり直しました。";
    }

    void DrawingCanvasPanel::drawUndoRedoControls()
    {
        ImGui::Text("Undo / Redo");
        ImGui::Text("対象: 描画、消しゴム、フレーム操作、レイヤー操作");
        ImGui::Text("履歴: Undo %d / Redo %d", static_cast<int>(undoStack_.size()), static_cast<int>(redoStack_.size()));

        // BeginDisabled() と EndDisabled() は必ず同じ条件で呼ぶ必要がある。
        // ボタンを押した結果 undoStack_ / redoStack_ の数が変わるので、
        // 先に canUndo / canRedo を保存してから使う。
        const bool canUndo = !undoStack_.empty();
        const bool canRedo = !redoStack_.empty();

        if (!canUndo)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("元に戻す Ctrl+Z") && canUndo)
        {
            undoLastAction();
        }

        if (!canUndo)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (!canRedo)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("やり直す Ctrl+Y") && canRedo)
        {
            redoLastAction();
        }

        if (!canRedo)
        {
            ImGui::EndDisabled();
        }

        if (!lastUndoRedoMessage_.empty())
        {
            ImGui::Text("%s", lastUndoRedoMessage_.c_str());
        }

        ImGui::Separator();
    }

    void DrawingCanvasPanel::updateUndoRedoShortcuts()
    {
        const ImGuiIO& io = ImGui::GetIO();

        if (!io.KeyCtrl)
        {
            return;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
        {
            undoLastAction();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
        {
            redoLastAction();
        }
    }

    void DrawingCanvasPanel::resetPlaybackProgress()
    {
        playbackSubFrameCounter_ = 0;
        playbackTimeAccumulatorSeconds_ = 0.0f;
    }

    void DrawingCanvasPanel::stopPlayback()
    {
        isPlaybackPlaying_ = false;
        resetPlaybackProgress();
    }

    void DrawingCanvasPanel::updatePlayback(const RenderFormat& renderFormat)
    {
        if (!isPlaybackPlaying_ || frames_.empty())
        {
            return;
        }

        const int playbackFps = std::clamp(
            renderFormat.framesPerSecond,
            1,
            240
        );

        const float secondsPerFrame = 1.0f / static_cast<float>(playbackFps);

        playbackTimeAccumulatorSeconds_ += ImGui::GetIO().DeltaTime;

        int safetyCounter = 0;

        while (playbackTimeAccumulatorSeconds_ >= secondsPerFrame && safetyCounter < 240)
        {
            playbackTimeAccumulatorSeconds_ -= secondsPerFrame;

            AnimationFrame* frame = activeFrame();

            if (frame == nullptr)
            {
                stopPlayback();
                return;
            }

            const int holdFrames = std::max(1, frame->durationFrames);

            ++playbackSubFrameCounter_;

            if (playbackSubFrameCounter_ >= holdFrames)
            {
                playbackSubFrameCounter_ = 0;

                if (activeFrameIndex_ < static_cast<int>(frames_.size()) - 1)
                {
                    ++activeFrameIndex_;
                    activeLayerIndex_ = 0;
                }
                else if (playbackLoopEnabled_)
                {
                    activeFrameIndex_ = 0;
                    activeLayerIndex_ = 0;
                }
                else
                {
                    stopPlayback();
                    return;
                }
            }

            ++safetyCounter;
        }
    }

    void DrawingCanvasPanel::addFrame()
    {
        pushUndoSnapshot("空フレーム追加");

        frames_.push_back(makeDefaultFrame(nextFrameNumber_));
        ++nextFrameNumber_;

        activeFrameIndex_ = static_cast<int>(frames_.size()) - 1;
        activeLayerIndex_ = 0;

        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::duplicateActiveFrame()
    {
        stopPlayback();

        const AnimationFrame* currentFrame = activeFrame();

        if (currentFrame == nullptr)
        {
            return;
        }

        pushUndoSnapshot("現在フレームを複製");

        AnimationFrame copiedFrame = *currentFrame;
        copiedFrame.name = makeDefaultFrameName(nextFrameNumber_);

        ++nextFrameNumber_;

        frames_.insert(
            frames_.begin() + activeFrameIndex_ + 1,
            copiedFrame
        );

        ++activeFrameIndex_;
        activeLayerIndex_ = 0;

        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::deleteActiveFrame()
    {
        stopPlayback();

        if (frames_.size() <= 1)
        {
            return;
        }

        clampActiveFrameIndex();

        pushUndoSnapshot("現在フレームを削除");

        frames_.erase(frames_.begin() + activeFrameIndex_);
        clampActiveFrameIndex();

        activeLayerIndex_ = 0;

        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::moveToPreviousFrame()
    {
        stopPlayback();

        if (activeFrameIndex_ > 0)
        {
            --activeFrameIndex_;
            activeLayerIndex_ = 0;
            currentStroke_.points.clear();
            isDrawing_ = false;
        }
    }

    void DrawingCanvasPanel::moveToNextFrame()
    {
        stopPlayback();

        if (activeFrameIndex_ < static_cast<int>(frames_.size()) - 1)
        {
            ++activeFrameIndex_;
            activeLayerIndex_ = 0;
            currentStroke_.points.clear();
            isDrawing_ = false;
        }
    }

    void DrawingCanvasPanel::addLayer()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            return;
        }

        pushUndoSnapshot("レイヤー追加");

        DrawingLayer layer;
        layer.name = makeDefaultLayerName(nextLayerNumber_);
        layer.visible = true;
        layer.opacity = 1.0f;

        ++nextLayerNumber_;

        frame->layers.push_back(layer);
        activeLayerIndex_ = static_cast<int>(frame->layers.size()) - 1;
    }

    void DrawingCanvasPanel::deleteActiveLayer()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->layers.size() <= 1)
        {
            return;
        }

        clampActiveLayerIndex();

        pushUndoSnapshot("レイヤー削除");

        frame->layers.erase(frame->layers.begin() + activeLayerIndex_);
        clampActiveLayerIndex();
    }

    void DrawingCanvasPanel::moveActiveLayerUp()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            return;
        }

        clampActiveLayerIndex();

        if (activeLayerIndex_ < 0 || activeLayerIndex_ >= static_cast<int>(frame->layers.size()) - 1)
        {
            return;
        }

        pushUndoSnapshot("レイヤーを上へ移動");

        std::swap(
            frame->layers[static_cast<std::size_t>(activeLayerIndex_)],
            frame->layers[static_cast<std::size_t>(activeLayerIndex_ + 1)]
        );

        ++activeLayerIndex_;
    }

    void DrawingCanvasPanel::moveActiveLayerDown()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            return;
        }

        clampActiveLayerIndex();

        if (activeLayerIndex_ <= 0)
        {
            return;
        }

        pushUndoSnapshot("レイヤーを下へ移動");

        std::swap(
            frame->layers[static_cast<std::size_t>(activeLayerIndex_)],
            frame->layers[static_cast<std::size_t>(activeLayerIndex_ - 1)]
        );

        --activeLayerIndex_;
    }

    void DrawingCanvasPanel::clearCurrentFrameLayers()
    {
        AnimationFrame* frame = activeFrame();

        if (frame == nullptr || frame->strokeCount() == 0)
        {
            return;
        }

        pushUndoSnapshot("現在フレームの全レイヤー消去");

        frame->clearAllLayers();
        currentStroke_.points.clear();
        isDrawing_ = false;
    }

    void DrawingCanvasPanel::exportCurrentRenderFramePng(
        const WorkCanvas& workCanvas,
        const RenderFormat& renderFormat
    )
    {
        const AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            lastPngExportSucceeded_ = false;
            lastPngExportMessage_ = "PNG保存失敗: 現在フレームがありません。";
            return;
        }

        std::ostringstream fileNameStream;

        fileNameStream
            << "frame_"
            << std::setw(4)
            << std::setfill('0')
            << nextPngExportNumber_
            << ".png";

        const std::filesystem::path outputPath =
            std::filesystem::path("exports") / fileNameStream.str();

        PngExportOptions options;
        options.transparentBackground = pngTransparentBackground_;

        std::string errorMessage;

        const bool succeeded = PngExporter::exportCenteredRenderFrame(
            outputPath,
            workCanvas,
            renderFormat,
            frame->layers,
            options,
            errorMessage
        );

        if (succeeded)
        {
            lastPngExportSucceeded_ = true;
            lastPngExportMessage_ = "PNG保存成功: " + outputPath.string();
            ++nextPngExportNumber_;
        }
        else
        {
            lastPngExportSucceeded_ = false;
            lastPngExportMessage_ = "PNG保存失敗: " + errorMessage;
        }
    }

    void DrawingCanvasPanel::exportAllFramesPngSequence(
        const WorkCanvas& workCanvas,
        const RenderFormat& renderFormat
    )
    {
        stopPlayback();
        currentStroke_.points.clear();
        isDrawing_ = false;

        if (frames_.empty())
        {
            lastPngSequenceExportSucceeded_ = false;
            lastPngSequenceExportMessage_ = "PNG連番保存失敗: フレームがありません。";
            return;
        }

        std::ostringstream directoryNameStream;

        directoryNameStream
            << "png_sequence_"
            << std::setw(4)
            << std::setfill('0')
            << nextPngSequenceExportNumber_;

        const std::filesystem::path outputDirectory =
            std::filesystem::path("exports") / directoryNameStream.str();

        std::error_code createDirectoryError;
        std::filesystem::create_directories(outputDirectory, createDirectoryError);

        if (createDirectoryError)
        {
            lastPngSequenceExportSucceeded_ = false;
            lastPngSequenceExportMessage_ =
                "PNG連番保存失敗: 出力フォルダを作れません: "
                + createDirectoryError.message();
            return;
        }

        PngExportOptions options;
        options.transparentBackground = pngTransparentBackground_;

        int outputFrameNumber = 1;

        for (std::size_t sourceFrameIndex = 0; sourceFrameIndex < frames_.size(); ++sourceFrameIndex)
        {
            const AnimationFrame& frame = frames_[sourceFrameIndex];
            const int holdFrames = std::max(1, frame.durationFrames);

            for (int holdFrameIndex = 0; holdFrameIndex < holdFrames; ++holdFrameIndex)
            {
                std::ostringstream fileNameStream;

                fileNameStream
                    << "frame_"
                    << std::setw(4)
                    << std::setfill('0')
                    << outputFrameNumber
                    << ".png";

                const std::filesystem::path outputPath =
                    outputDirectory / fileNameStream.str();

                std::string errorMessage;

                const bool succeeded = PngExporter::exportCenteredRenderFrame(
                    outputPath,
                    workCanvas,
                    renderFormat,
                    frame.layers,
                    options,
                    errorMessage
                );

                if (!succeeded)
                {
                    lastPngSequenceExportSucceeded_ = false;
                    lastPngSequenceExportMessage_ =
                        "PNG連番保存失敗: "
                        + frame.name
                        + " の書き出し中に失敗: "
                        + errorMessage;
                    return;
                }

                ++outputFrameNumber;
            }
        }

        const int exportedImageCount = outputFrameNumber - 1;

        lastPngSequenceExportSucceeded_ = true;
        lastPngSequenceExportMessage_ =
            "PNG連番保存成功: "
            + outputDirectory.string()
            + " / "
            + std::to_string(exportedImageCount)
            + "枚";


        ++nextPngSequenceExportNumber_;
    }

    void DrawingCanvasPanel::saveProjectFile(
        const WorkCanvas& workCanvas,
        const RenderFormat& renderFormat
    )
    {
        stopPlayback();
        currentStroke_.points.clear();
        isDrawing_ = false;

        std::error_code createDirectoryError;
        const std::filesystem::path projectPath =
            makeProjectFilePathFromProjectRoot(projectFilePath_);

        if (projectPath.has_parent_path())
        {
            std::filesystem::create_directories(
                projectPath.parent_path(),
                createDirectoryError
            );
        }

        if (createDirectoryError)
        {
            lastProjectFileSucceeded_ = false;
            lastProjectFileMessage_ =
                "プロジェクト保存失敗: フォルダを作れません: "
                + createDirectoryError.message();
            return;
        }

        std::ofstream output(projectPath);

        if (!output)
        {
            lastProjectFileSucceeded_ = false;
            lastProjectFileMessage_ =
                "プロジェクト保存失敗: ファイルを開けません: "
                + projectPath.string();
            return;
        }

        writeProjectFileToStream(
            output,
            workCanvas,
            renderFormat,
            brush_,
            frames_
        );

        if (!output)
        {
            lastProjectFileSucceeded_ = false;
            lastProjectFileMessage_ = "プロジェクト保存失敗: 書き込み中に失敗しました。";
            return;
        }

        lastProjectFileSucceeded_ = true;
        lastProjectFileMessage_ = "プロジェクト保存成功: " + projectPath.string();
    }

    void DrawingCanvasPanel::loadProjectFile(
        WorkCanvas& workCanvas,
        RenderFormat& renderFormat
    )
    {
        stopPlayback();
        currentStroke_.points.clear();
        isDrawing_ = false;

        const std::filesystem::path projectPath =
            makeProjectFilePathFromProjectRoot(projectFilePath_);

        std::ifstream input(projectPath);

        if (!input)
        {
            lastProjectFileSucceeded_ = false;
            lastProjectFileMessage_ =
                "プロジェクト読み込み失敗: ファイルを開けません: "
                + projectPath.string();
            return;
        }

        std::string errorMessage;
        Brush loadedBrush = brush_;
        std::vector<AnimationFrame> loadedFrames;

        const bool succeeded = readProjectFileFromStream(
            input,
            workCanvas,
            renderFormat,
            loadedBrush,
            loadedFrames,
            errorMessage
        );

        if (!succeeded)
        {
            lastProjectFileSucceeded_ = false;
            lastProjectFileMessage_ = errorMessage;
            return;
        }

        brush_ = loadedBrush;
        frames_ = loadedFrames;

        if (frames_.empty())
        {
            frames_.push_back(makeDefaultFrame(1));
        }

        activeFrameIndex_ = 0;
        activeLayerIndex_ = 0;
        nextFrameNumber_ = static_cast<int>(frames_.size()) + 1;

        int largestLayerCount = 0;

        for (const AnimationFrame& frame : frames_)
        {
            largestLayerCount = std::max(
                largestLayerCount,
                static_cast<int>(frame.layers.size())
            );
        }

        nextLayerNumber_ = largestLayerCount + 1;

        clearHistory();

        lastProjectFileSucceeded_ = true;
        lastProjectFileMessage_ = "プロジェクト読み込み成功: " + projectPath.string();
    }

    void DrawingCanvasPanel::drawFramePanel(const RenderFormat& renderFormat)
    {
        ImGui::Begin("フレーム");

        ImGui::Text("現在のフレームを選びます。");
        ImGui::Text("Phase 3Lではフレーム名・レイヤー名を変更できます。");

        ImGui::Separator();

        ImGui::Text("オニオンスキン");

        ImGui::Checkbox("オニオンスキンを表示", &onionSkinEnabled_);

        if (onionSkinEnabled_)
        {
            ImGui::Checkbox("前フレームを表示", &showPreviousOnionSkin_);
            ImGui::Checkbox("次フレームを表示", &showNextOnionSkin_);

            ImGui::SetNextItemWidth(160.0f);
            ImGui::SliderInt("表示範囲", &onionSkinRange_, 1, 3);

            ImGui::SetNextItemWidth(160.0f);
            ImGui::SliderFloat("濃さ", &onionSkinOpacity_, 0.05f, 0.8f);

            ImGui::Checkbox("非表示レイヤーは除外", &onionSkinVisibleLayersOnly_);
        }

        ImGui::Separator();

        ImGui::Text("再生プレビュー");
        ImGui::Text("再生FPS: %d", renderFormat.framesPerSecond);

        ImGui::Checkbox("ループ再生", &playbackLoopEnabled_);

        if (ImGui::Button(isPlaybackPlaying_ ? "停止" : "再生"))
        {
            if (isPlaybackPlaying_)
            {
                stopPlayback();
            }
            else
            {
                resetPlaybackProgress();
                currentStroke_.points.clear();
                isDrawing_ = false;
                isPlaybackPlaying_ = true;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("先頭へ"))
        {
            stopPlayback();
            activeFrameIndex_ = 0;
            activeLayerIndex_ = 0;
            currentStroke_.points.clear();
            isDrawing_ = false;
        }

        if (isPlaybackPlaying_)
        {
            ImGui::TextColored(
                ImVec4(0.45f, 1.0f, 0.55f, 1.0f),
                "再生中: 描画は一時停止します。"
            );
        }

        ImGui::Separator();

        if (ImGui::Button("前のフレーム"))
        {
            moveToPreviousFrame();
        }

        ImGui::SameLine();

        if (ImGui::Button("次のフレーム"))
        {
            moveToNextFrame();
        }

        if (ImGui::Button("空フレーム追加"))
        {
            addFrame();
        }

        ImGui::SameLine();

        if (ImGui::Button("現在フレームを複製"))
        {
            duplicateActiveFrame();
        }

        if (ImGui::Button("現在フレームを削除"))
        {
            deleteActiveFrame();
        }

        ImGui::Separator();

        for (int index = 0; index < static_cast<int>(frames_.size()); ++index)
        {
            AnimationFrame& frame = frames_[static_cast<std::size_t>(index)];

            ImGui::PushID(index);

            const bool isActive = (index == activeFrameIndex_);

            if (ImGui::RadioButton(frame.name.c_str(), isActive))
            {
                stopPlayback();

                activeFrameIndex_ = index;
                activeLayerIndex_ = 0;
                currentStroke_.points.clear();
                isDrawing_ = false;
            }

            char frameNameBuffer[128] = {};
            copyStringToInputBuffer(
                frame.name,
                frameNameBuffer,
                sizeof(frameNameBuffer)
            );

            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::InputText("フレーム名", frameNameBuffer, sizeof(frameNameBuffer)))
            {
                const std::string newFrameName = normalizeEditableName(
                    frameNameBuffer,
                    makeDefaultFrameName(index + 1)
                );

                if (newFrameName != frame.name)
                {
                    pushUndoSnapshot("フレーム名変更");
                    frame.name = newFrameName;
                }
            }

            ImGui::Text("ストローク数: %d", frame.strokeCount());

            int editedDurationFrames = frame.durationFrames;
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::SliderInt("保持コマ数", &editedDurationFrames, 1, 24)
                && editedDurationFrames != frame.durationFrames)
            {
                pushUndoSnapshot("保持コマ数変更");
                frame.durationFrames = editedDurationFrames;
            }

            ImGui::Separator();

            ImGui::PopID();
        }

        const AnimationFrame* frame = activeFrame();

        if (frame != nullptr)
        {
            ImGui::Text("現在フレーム: %s", frame->name.c_str());
        }

        ImGui::End();
    }

    void DrawingCanvasPanel::drawTimelinePanel(const RenderFormat& renderFormat)
    {
        ImGui::Begin("タイムライン");

        if (frames_.empty())
        {
            ImGui::Text("フレームがありません。");
            ImGui::End();
            return;
        }

        const int totalTimelineFrames = calculatePngSequenceImageCount(frames_);
        const int playbackFps = std::clamp(renderFormat.framesPerSecond, 1, 240);
        const float totalSeconds =
            static_cast<float>(totalTimelineFrames) / static_cast<float>(playbackFps);

        ImGui::Text(
            "合計: %dコマ / %.2f秒 / %d fps",
            totalTimelineFrames,
            totalSeconds,
            playbackFps
        );

        ImGui::Text(
            "現在: Frame %d / %d",
            activeFrameIndex_ + 1,
            static_cast<int>(frames_.size())
        );

        if (ImGui::Button("先頭"))
        {
            stopPlayback();
            activeFrameIndex_ = 0;
            activeLayerIndex_ = 0;
            currentStroke_.points.clear();
            isDrawing_ = false;
            isErasing_ = false;
            hasPreviousEraserPoint_ = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("前"))
        {
            moveToPreviousFrame();
        }

        ImGui::SameLine();

        if (ImGui::Button("次"))
        {
            moveToNextFrame();
        }

        ImGui::SameLine();

        if (ImGui::Button(isPlaybackPlaying_ ? "停止" : "再生"))
        {
            if (isPlaybackPlaying_)
            {
                stopPlayback();
            }
            else
            {
                resetPlaybackProgress();
                currentStroke_.points.clear();
                isDrawing_ = false;
                isErasing_ = false;
                hasPreviousEraserPoint_ = false;
                isPlaybackPlaying_ = true;
            }
        }

        ImGui::SameLine();
        ImGui::Checkbox("ループ", &playbackLoopEnabled_);

        AnimationFrame* currentFrame = activeFrame();

        if (currentFrame != nullptr)
        {
            int editedDurationFrames = currentFrame->durationFrames;
            ImGui::SetNextItemWidth(180.0f);

            if (ImGui::SliderInt("選択フレームの保持コマ数", &editedDurationFrames, 1, 24)
                && editedDurationFrames != currentFrame->durationFrames)
            {
                pushUndoSnapshot("タイムライン保持コマ数変更");
                currentFrame->durationFrames = editedDurationFrames;
            }
        }

        ImGui::Separator();

        ImGui::Text("横長の箱がフレームです。箱の幅が保持コマ数を表します。");

        const ImVec2 childSize(0.0f, 140.0f);

        ImGui::BeginChild(
            "TimelineScrollArea",
            childSize,
            true,
            ImGuiWindowFlags_HorizontalScrollbar
        );

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        constexpr float PixelsPerHeldFrame = 28.0f;
        constexpr float MinimumFrameBlockWidth = 56.0f;
        constexpr float FrameBlockHeight = 52.0f;
        constexpr float FrameBlockGap = 6.0f;

        for (int index = 0; index < static_cast<int>(frames_.size()); ++index)
        {
            AnimationFrame& frame = frames_[static_cast<std::size_t>(index)];
            const int holdFrames = std::max(1, frame.durationFrames);
            const float blockWidth = std::max(
                MinimumFrameBlockWidth,
                static_cast<float>(holdFrames) * PixelsPerHeldFrame
            );

            ImGui::PushID(index);

            ImGui::InvisibleButton(
                "TimelineFrameBlock",
                ImVec2(blockWidth, FrameBlockHeight)
            );

            const ImVec2 blockMin = ImGui::GetItemRectMin();
            const ImVec2 blockMax = ImGui::GetItemRectMax();
            const bool isHovered = ImGui::IsItemHovered();
            const bool isActive = (index == activeFrameIndex_);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                stopPlayback();
                activeFrameIndex_ = index;
                activeLayerIndex_ = 0;
                currentStroke_.points.clear();
                isDrawing_ = false;
                isErasing_ = false;
                hasPreviousEraserPoint_ = false;
            }

            const ImU32 fillColor = isActive
                ? IM_COL32(70, 120, 190, 255)
                : IM_COL32(50, 54, 64, 255);

            const ImU32 borderColor = isActive
                ? IM_COL32(255, 220, 80, 255)
                : IM_COL32(130, 140, 160, 255);

            const ImU32 hoveredBorderColor = IM_COL32(230, 240, 255, 255);

            drawList->AddRectFilled(blockMin, blockMax, fillColor, 6.0f);
            drawList->AddRect(
                blockMin,
                blockMax,
                isHovered ? hoveredBorderColor : borderColor,
                6.0f,
                0,
                isActive ? 3.0f : 1.5f
            );

            const std::string frameLabel =
                std::to_string(index + 1) + ": " + frame.name;

            const std::string durationLabel =
                std::to_string(holdFrames) + "コマ";

            drawList->AddText(
                ImVec2(blockMin.x + 8.0f, blockMin.y + 8.0f),
                IM_COL32(255, 255, 255, 255),
                frameLabel.c_str()
            );

            drawList->AddText(
                ImVec2(blockMin.x + 8.0f, blockMin.y + 28.0f),
                IM_COL32(210, 220, 235, 255),
                durationLabel.c_str()
            );

            if (isActive)
            {
                float markerRatio = 0.0f;

                if (isPlaybackPlaying_)
                {
                    markerRatio = std::clamp(
                        static_cast<float>(playbackSubFrameCounter_) /
                            static_cast<float>(holdFrames),
                        0.0f,
                        1.0f
                    );
                }

                const float markerX = blockMin.x + blockWidth * markerRatio;

                drawList->AddLine(
                    ImVec2(markerX, blockMin.y - 8.0f),
                    ImVec2(markerX, blockMax.y + 8.0f),
                    IM_COL32(255, 80, 80, 255),
                    3.0f
                );
            }

            ImGui::PopID();

            if (index + 1 < static_cast<int>(frames_.size()))
            {
                ImGui::SameLine(0.0f, FrameBlockGap);
            }
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void DrawingCanvasPanel::drawLayerPanel()
    {
        ImGui::Begin("レイヤー");

        AnimationFrame* frame = activeFrame();

        if (frame == nullptr)
        {
            ImGui::Text("現在フレームがありません。");
            ImGui::End();
            return;
        }

        ImGui::Text("現在フレーム内のレイヤーを管理します。");
        ImGui::Text("上のレイヤーほど手前に表示されます。");

        ImGui::Separator();

        if (ImGui::Button("レイヤー追加"))
        {
            addLayer();
        }

        ImGui::SameLine();

        if (ImGui::Button("削除"))
        {
            deleteActiveLayer();
        }

        if (ImGui::Button("上へ"))
        {
            moveActiveLayerUp();
        }

        ImGui::SameLine();

        if (ImGui::Button("下へ"))
        {
            moveActiveLayerDown();
        }

        ImGui::Separator();

        for (int index = static_cast<int>(frame->layers.size()) - 1; index >= 0; --index)
        {
            DrawingLayer& layer = frame->layers[static_cast<std::size_t>(index)];

            ImGui::PushID(index);

            const bool isActive = (index == activeLayerIndex_);

            char layerNameBuffer[128] = {};
            copyStringToInputBuffer(
                layer.name,
                layerNameBuffer,
                sizeof(layerNameBuffer)
            );

            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::InputText("レイヤー名", layerNameBuffer, sizeof(layerNameBuffer)))
            {
                const std::string newLayerName = normalizeEditableName(
                    layerNameBuffer,
                    makeDefaultLayerName(index + 1)
                );

                if (newLayerName != layer.name)
                {
                    pushUndoSnapshot("レイヤー名変更");
                    layer.name = newLayerName;
                }
            }

            if (ImGui::RadioButton("このレイヤーに描く", isActive))
            {
                activeLayerIndex_ = index;
            }

            bool editedVisible = layer.visible;
            if (ImGui::Checkbox("表示する", &editedVisible)
                && editedVisible != layer.visible)
            {
                pushUndoSnapshot("レイヤー表示切り替え");
                layer.visible = editedVisible;
            }

            float editedOpacity = layer.opacity;
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::SliderFloat("不透明度", &editedOpacity, 0.0f, 1.0f)
                && editedOpacity != layer.opacity)
            {
                pushUndoSnapshot("レイヤー不透明度変更");
                layer.opacity = editedOpacity;
            }

            ImGui::Text("ストローク数: %d", static_cast<int>(layer.strokes.size()));

            if (isActive)
            {
                ImGui::TextColored(
                    ImVec4(0.45f, 0.80f, 1.0f, 1.0f),
                    "現在の描き込み先"
                );
            }

            ImGui::Separator();

            ImGui::PopID();
        }

        DrawingLayer* layer = activeLayer();

        if (layer != nullptr)
        {
            ImGui::Text("現在の描き込み先: %s", layer->name.c_str());

            if (!layer->visible)
            {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                    "注意: このレイヤーは非表示なので描き込みできません。"
                );
            }

            if (ImGui::Button("アクティブレイヤーを消去"))
            {
                if (layer->hasStrokes())
                {
                    pushUndoSnapshot("アクティブレイヤーを消去");
                    layer->clear();
                }
            }
        }

        if (ImGui::Button("現在フレームの全レイヤー消去"))
        {
            clearCurrentFrameLayers();
        }

        ImGui::End();
    }

    void DrawingCanvasPanel::draw(WorkCanvas& workCanvas, RenderFormat& renderFormat)
    {
        updateUndoRedoShortcuts();
        updatePlayback(renderFormat);

        drawFramePanel(renderFormat);
        drawTimelinePanel(renderFormat);
        drawLayerPanel();

        ImGui::Begin("簡易作画キャンバス");

        const AnimationFrame* currentFrame = activeFrame();

        if (currentFrame != nullptr)
        {
            ImGui::Text("現在フレーム: %s", currentFrame->name.c_str());
        }

        ImGui::Text("左ドラッグで、ペン描画または消しゴム操作ができます。");

        drawUndoRedoControls();

        ImGui::Text("ツール");

        int drawingToolIndex = (drawingTool_ == DrawingTool::Pen) ? 0 : 1;

        if (ImGui::RadioButton("ペン", drawingToolIndex == 0))
        {
            drawingTool_ = DrawingTool::Pen;
            currentStroke_.points.clear();
            isDrawing_ = false;
            isErasing_ = false;
            hasPreviousEraserPoint_ = false;
        }

        ImGui::SameLine();

        if (ImGui::RadioButton("消しゴム", drawingToolIndex == 1))
        {
            drawingTool_ = DrawingTool::Eraser;
            currentStroke_.points.clear();
            isDrawing_ = false;
            isErasing_ = false;
            hasPreviousEraserPoint_ = false;
        }

        ImGui::SliderFloat("ペン半径 px", &brush_.radiusPx, 1.0f, 40.0f);

        float color[4] = {
            brush_.color.r,
            brush_.color.g,
            brush_.color.b,
            brush_.color.a
        };

        if (ImGui::ColorEdit4("ペン色", color))
        {
            brush_.color.r = color[0];
            brush_.color.g = color[1];
            brush_.color.b = color[2];
            brush_.color.a = color[3];
        }

        ImGui::SliderFloat("消しゴム半径 px", &eraserRadiusPx_, 4.0f, 120.0f);

        ImGui::Separator();

        ImGui::Text("PNG保存");

        ImGui::Checkbox("透明背景で保存", &pngTransparentBackground_);

        if (ImGui::Button("現在の撮影フレームをPNG保存"))
        {
            exportCurrentRenderFramePng(workCanvas, renderFormat);
        }

        if (!lastPngExportMessage_.empty())
        {
            if (lastPngExportSucceeded_)
            {
                ImGui::TextColored(
                    ImVec4(0.45f, 1.0f, 0.55f, 1.0f),
                    "%s",
                    lastPngExportMessage_.c_str()
                );
            }
            else
            {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                    "%s",
                    lastPngExportMessage_.c_str()
                );
            }
        }

        ImGui::Spacing();
        ImGui::Text("PNG連番保存");
        ImGui::Text("保持コマ数を反映して全フレームを書き出します。");
        ImGui::Text("出力予定枚数: %d", calculatePngSequenceImageCount(frames_));

        if (ImGui::Button("全フレームをPNG連番保存"))
        {
            exportAllFramesPngSequence(workCanvas, renderFormat);
        }

        if (!lastPngSequenceExportMessage_.empty())
        {
            if (lastPngSequenceExportSucceeded_)
            {
                ImGui::TextColored(
                    ImVec4(0.45f, 1.0f, 0.55f, 1.0f),
                    "%s",
                    lastPngSequenceExportMessage_.c_str()
                );
            }
            else
            {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                    "%s",
                    lastPngSequenceExportMessage_.c_str()
                );
            }
        }

        ImGui::Spacing();
        ImGui::Text("プロジェクト保存/読み込み");
        ImGui::Text("保存先: %s", projectFilePath_.c_str());
        ImGui::Text("読み込み時は、現在の作業内容をファイル内容で置き換えます。");

        if (ImGui::Button("プロジェクト保存"))
        {
            saveProjectFile(workCanvas, renderFormat);
        }

        ImGui::SameLine();

        if (ImGui::Button("プロジェクト読み込み"))
        {
            loadProjectFile(workCanvas, renderFormat);
        }

        if (!lastProjectFileMessage_.empty())
        {
            if (lastProjectFileSucceeded_)
            {
                ImGui::TextColored(
                    ImVec4(0.45f, 1.0f, 0.55f, 1.0f),
                    "%s",
                    lastProjectFileMessage_.c_str()
                );
            }
            else
            {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.45f, 0.25f, 1.0f),
                    "%s",
                    lastProjectFileMessage_.c_str()
                );
            }
        }

        ImGui::Separator();

        DrawingLayer* currentLayer = activeLayer();

        if (currentLayer != nullptr)
        {
            ImGui::Text("描き込み先: %s", currentLayer->name.c_str());
        }

        if (isPlaybackPlaying_)
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.75f, 0.25f, 1.0f),
                "再生中は描画できません。停止してから描いてください。"
            );
        }

        ImGui::Separator();

        ImVec2 availableSize = ImGui::GetContentRegionAvail();
        availableSize.y = std::max(availableSize.y, 420.0f);

        ImGui::BeginChild(
            "DrawingCanvasArea",
            availableSize,
            true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        const ImVec2 previewMin = ImGui::GetCursorScreenPos();
        const ImVec2 previewSize = ImGui::GetContentRegionAvail();
        const ImVec2 previewMax(
            previewMin.x + previewSize.x,
            previewMin.y + previewSize.y
        );

        ImGui::InvisibleButton(
            "DrawingCanvasInput",
            previewSize,
            ImGuiButtonFlags_MouseButtonLeft
        );

        const bool isInputAreaHovered = ImGui::IsItemHovered();

        drawList->AddRectFilled(previewMin, previewMax, PreviewBackgroundColor);

        constexpr float PaddingPx = 24.0f;

        const float availableWidth = std::max(1.0f, previewSize.x - PaddingPx * 2.0f);
        const float availableHeight = std::max(1.0f, previewSize.y - PaddingPx * 2.0f);

        const float scaleX = availableWidth / static_cast<float>(workCanvas.widthPx);
        const float scaleY = availableHeight / static_cast<float>(workCanvas.heightPx);
        const float scale = std::min(scaleX, scaleY);

        const float canvasPreviewWidth = static_cast<float>(workCanvas.widthPx) * scale;
        const float canvasPreviewHeight = static_cast<float>(workCanvas.heightPx) * scale;

        const ImVec2 canvasMin(
            previewMin.x + (previewSize.x - canvasPreviewWidth) * 0.5f,
            previewMin.y + (previewSize.y - canvasPreviewHeight) * 0.5f
        );

        const ImVec2 canvasMax(
            canvasMin.x + canvasPreviewWidth,
            canvasMin.y + canvasPreviewHeight
        );

        drawList->AddRectFilled(canvasMin, canvasMax, CanvasFillColor);
        drawList->AddRect(canvasMin, canvasMax, CanvasBorderColor, 0.0f, 0, 2.0f);

        constexpr int GridSpacingCanvasPx = 200;

        for (int x = 0; x <= workCanvas.widthPx; x += GridSpacingCanvasPx)
        {
            const float screenX = canvasMin.x + static_cast<float>(x) * scale;
            drawList->AddLine(
                ImVec2(screenX, canvasMin.y),
                ImVec2(screenX, canvasMax.y),
                GridLineColor
            );
        }

        for (int y = 0; y <= workCanvas.heightPx; y += GridSpacingCanvasPx)
        {
            const float screenY = canvasMin.y + static_cast<float>(y) * scale;
            drawList->AddLine(
                ImVec2(canvasMin.x, screenY),
                ImVec2(canvasMax.x, screenY),
                GridLineColor
            );
        }

        drawList->PushClipRect(canvasMin, canvasMax, true);

        if (onionSkinEnabled_ && !frames_.empty())
        {
            onionSkinRange_ = std::clamp(onionSkinRange_, 1, 3);
            onionSkinOpacity_ = std::clamp(onionSkinOpacity_, 0.0f, 1.0f);

            for (int offset = onionSkinRange_; offset >= 1; --offset)
            {
                const float fadedOpacity =
                    onionSkinOpacity_ / static_cast<float>(offset);

                const int previousFrameIndex = activeFrameIndex_ - offset;
                const int nextFrameIndex = activeFrameIndex_ + offset;

                if (showPreviousOnionSkin_ && previousFrameIndex >= 0)
                {
                    drawOnionSkinFrame(
                        drawList,
                        frames_[static_cast<std::size_t>(previousFrameIndex)],
                        canvasMin,
                        scale,
                        true,
                        fadedOpacity,
                        onionSkinVisibleLayersOnly_
                    );
                }

                if (showNextOnionSkin_ && nextFrameIndex < static_cast<int>(frames_.size()))
                {
                    drawOnionSkinFrame(
                        drawList,
                        frames_[static_cast<std::size_t>(nextFrameIndex)],
                        canvasMin,
                        scale,
                        false,
                        fadedOpacity,
                        onionSkinVisibleLayersOnly_
                    );
                }
            }
        }

        if (currentFrame != nullptr)
        {
            for (const DrawingLayer& layer : currentFrame->layers)
            {
                if (!layer.visible)
                {
                    continue;
                }

                for (const Stroke& stroke : layer.strokes)
                {
                    drawStroke(drawList, stroke, canvasMin, scale, layer.opacity);
                }
            }
        }

        if (drawingTool_ == DrawingTool::Pen
            && isDrawing_
            && currentLayer != nullptr
            && currentLayer->visible)
        {
            drawStroke(drawList, currentStroke_, canvasMin, scale, currentLayer->opacity);
        }

        drawList->PopClipRect();

        const float renderFramePreviewWidth =
            static_cast<float>(renderFormat.outputWidthPx) * scale;

        const float renderFramePreviewHeight =
            static_cast<float>(renderFormat.outputHeightPx) * scale;

        const ImVec2 renderFrameMin(
            canvasMin.x + (canvasPreviewWidth - renderFramePreviewWidth) * 0.5f,
            canvasMin.y + (canvasPreviewHeight - renderFramePreviewHeight) * 0.5f
        );

        const ImVec2 renderFrameMax(
            renderFrameMin.x + renderFramePreviewWidth,
            renderFrameMin.y + renderFramePreviewHeight
        );

        const bool renderFrameFits =
            renderFrameMin.x >= canvasMin.x
            && renderFrameMin.y >= canvasMin.y
            && renderFrameMax.x <= canvasMax.x
            && renderFrameMax.y <= canvasMax.y;

        drawList->AddRect(
            renderFrameMin,
            renderFrameMax,
            renderFrameFits ? RenderFrameColor : WarningColor,
            0.0f,
            0,
            3.0f
        );

        drawList->AddText(
            ImVec2(renderFrameMin.x + 8.0f, renderFrameMin.y + 8.0f),
            renderFrameFits ? RenderFrameColor : WarningColor,
            "撮影フレーム"
        );

        const ImVec2 mousePosition = ImGui::GetIO().MousePos;
        const bool mouseInsideCanvas = isInsideRect(mousePosition, canvasMin, canvasMax);

        if (drawingTool_ == DrawingTool::Eraser
            && isInputAreaHovered
            && mouseInsideCanvas)
        {
            drawList->AddCircle(
                mousePosition,
                eraserRadiusPx_ * scale,
                IM_COL32(255, 130, 90, 220),
                32,
                2.0f
            );
        }

        const bool canEditCanvas =
            currentLayer != nullptr
            && currentLayer->visible
            && !isPlaybackPlaying_;

        if (canEditCanvas
            && drawingTool_ == DrawingTool::Pen
            && isInputAreaHovered
            && mouseInsideCanvas
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            currentStroke_ = Stroke{};
            currentStroke_.color = brush_.color;
            currentStroke_.radiusPx = brush_.radiusPx;
            currentStroke_.addPoint(screenToCanvasPoint(mousePosition, canvasMin, scale));
            isDrawing_ = true;
            isErasing_ = false;
        }

        if (canEditCanvas
            && drawingTool_ == DrawingTool::Eraser
            && isInputAreaHovered
            && mouseInsideCanvas
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            pushUndoSnapshot("消しゴム");
            currentStroke_.points.clear();
            isDrawing_ = false;
            isErasing_ = true;

            previousEraserPoint_ = screenToCanvasPoint(mousePosition, canvasMin, scale);
            hasPreviousEraserPoint_ = true;

            eraseActiveLayerAt(previousEraserPoint_);
        }

        if (canEditCanvas
            && drawingTool_ == DrawingTool::Pen
            && isDrawing_
            && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (mouseInsideCanvas)
            {
                const CanvasPoint nextPoint =
                    screenToCanvasPoint(mousePosition, canvasMin, scale);

                constexpr float MinimumPointDistancePx = 2.0f;

                if (currentStroke_.points.empty()
                    || distanceSquared(currentStroke_.points.back(), nextPoint)
                        >= MinimumPointDistancePx * MinimumPointDistancePx)
                {
                    currentStroke_.addPoint(nextPoint);
                }
            }
        }

        if (canEditCanvas
            && drawingTool_ == DrawingTool::Eraser
            && isErasing_
            && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (mouseInsideCanvas)
            {
                const CanvasPoint currentEraserPoint =
                    screenToCanvasPoint(mousePosition, canvasMin, scale);

                if (hasPreviousEraserPoint_)
                {
                    eraseActiveLayerAlongPath(previousEraserPoint_, currentEraserPoint);
                }
                else
                {
                    eraseActiveLayerAt(currentEraserPoint);
                }

                previousEraserPoint_ = currentEraserPoint;
                hasPreviousEraserPoint_ = true;
            }
        }

        if (isDrawing_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            if (canEditCanvas && currentStroke_.hasDrawablePoints())
            {
                pushUndoSnapshot("ストローク描画");
                currentLayer->strokes.push_back(currentStroke_);
            }

            currentStroke_.points.clear();
            isDrawing_ = false;
        }

        if (isErasing_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            isErasing_ = false;
            hasPreviousEraserPoint_ = false;
        }

        ImGui::EndChild();
        ImGui::End();
    }
}
