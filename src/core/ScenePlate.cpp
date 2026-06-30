// このファイルの役割:
// Scene Plate 最小モデルの文字列変換、正規化、表示/出力判定を実装する。

#include "core/ScenePlate.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace perapera {
namespace {

bool equals(std::string_view lhs, std::string_view rhs) noexcept
{
    return lhs == rhs;
}

float safeScale(float value) noexcept
{
    if (!std::isfinite(value)) {
        return 1.0f;
    }
    if (value == 0.0f) {
        return 1.0f;
    }
    return value;
}

float safeFloat(float value, float fallback) noexcept
{
    return std::isfinite(value) ? value : fallback;
}

} // namespace

const char* scenePlateKindToString(ScenePlateKind kind) noexcept
{
    switch (kind) {
    case ScenePlateKind::Storyboard:
        return "storyboard";
    case ScenePlateKind::Layout:
        return "layout";
    case ScenePlateKind::ReferenceImage:
        return "referenceImage";
    case ScenePlateKind::TemporaryBackground:
        return "temporaryBackground";
    case ScenePlateKind::FinalBackground:
        return "finalBackground";
    }
    return "storyboard";
}

ScenePlateKind scenePlateKindFromString(std::string_view value) noexcept
{
    if (equals(value, "layout")) {
        return ScenePlateKind::Layout;
    }
    if (equals(value, "referenceImage")) {
        return ScenePlateKind::ReferenceImage;
    }
    if (equals(value, "temporaryBackground")) {
        return ScenePlateKind::TemporaryBackground;
    }
    if (equals(value, "finalBackground")) {
        return ScenePlateKind::FinalBackground;
    }
    return ScenePlateKind::Storyboard;
}

const char* scenePlateKindJapaneseLabel(ScenePlateKind kind) noexcept
{
    switch (kind) {
    case ScenePlateKind::Storyboard:
        return "絵コンテ";
    case ScenePlateKind::Layout:
        return "レイアウト";
    case ScenePlateKind::ReferenceImage:
        return "参照画像";
    case ScenePlateKind::TemporaryBackground:
        return "仮背景";
    case ScenePlateKind::FinalBackground:
        return "完成背景";
    }
    return "絵コンテ";
}

const char* scenePlateOutputModeToString(ScenePlateOutputMode mode) noexcept
{
    switch (mode) {
    case ScenePlateOutputMode::ReferenceOnly:
        return "referenceOnly";
    case ScenePlateOutputMode::PreviewOnly:
        return "previewOnly";
    case ScenePlateOutputMode::RenderOutput:
        return "renderOutput";
    }
    return "referenceOnly";
}

ScenePlateOutputMode scenePlateOutputModeFromString(std::string_view value) noexcept
{
    if (equals(value, "previewOnly")) {
        return ScenePlateOutputMode::PreviewOnly;
    }
    if (equals(value, "renderOutput")) {
        return ScenePlateOutputMode::RenderOutput;
    }
    return ScenePlateOutputMode::ReferenceOnly;
}

const char* scenePlateOutputModeJapaneseLabel(ScenePlateOutputMode mode) noexcept
{
    switch (mode) {
    case ScenePlateOutputMode::ReferenceOnly:
        return "参照のみ";
    case ScenePlateOutputMode::PreviewOnly:
        return "プレビューのみ";
    case ScenePlateOutputMode::RenderOutput:
        return "出力対象";
    }
    return "参照のみ";
}

float clampScenePlateOpacity(float opacity) noexcept
{
    if (!std::isfinite(opacity)) {
        return 0.5f;
    }
    if (opacity < 0.0f) {
        return 0.0f;
    }
    if (opacity > 1.0f) {
        return 1.0f;
    }
    return opacity;
}

void normalizeScenePlate(ScenePlate& plate)
{
    if (plate.displayName.empty()) {
        plate.displayName = plate.id.empty() ? "Scene Plate" : plate.id;
    }

    plate.opacity = clampScenePlateOpacity(plate.opacity);

    if (plate.startTimelineFrame < 0) {
        plate.startTimelineFrame = 0;
    }
    if (plate.endTimelineFrame < 0) {
        plate.endTimelineFrame = 0;
    }
    if (plate.startTimelineFrame > 0 && plate.endTimelineFrame > 0 && plate.endTimelineFrame < plate.startTimelineFrame) {
        std::swap(plate.startTimelineFrame, plate.endTimelineFrame);
    }

    plate.transform.x = safeFloat(plate.transform.x, 0.0f);
    plate.transform.y = safeFloat(plate.transform.y, 0.0f);
    plate.transform.scaleX = safeScale(plate.transform.scaleX);
    plate.transform.scaleY = safeScale(plate.transform.scaleY);
    plate.transform.rotationDegrees = safeFloat(plate.transform.rotationDegrees, 0.0f);
}

void normalizeScenePlateStack(ScenePlateStack& stack)
{
    std::unordered_set<std::string> usedIds;
    int generatedId = 1;

    for (ScenePlate& plate : stack.plates) {
        if (plate.id.empty() || usedIds.find(plate.id) != usedIds.end()) {
            do {
                plate.id = "plate_" + std::to_string(generatedId++);
            } while (usedIds.find(plate.id) != usedIds.end());
        }
        usedIds.insert(plate.id);
        normalizeScenePlate(plate);
    }

    std::stable_sort(stack.plates.begin(), stack.plates.end(), [](const ScenePlate& lhs, const ScenePlate& rhs) {
        return lhs.zOrder < rhs.zOrder;
    });
}

bool scenePlateVisibleAtTimelineFrame(const ScenePlate& plate, int timelineFrame) noexcept
{
    if (!plate.visible) {
        return false;
    }
    if (timelineFrame <= 0) {
        return true;
    }
    if (plate.startTimelineFrame > 0 && timelineFrame < plate.startTimelineFrame) {
        return false;
    }
    if (plate.endTimelineFrame > 0 && timelineFrame > plate.endTimelineFrame) {
        return false;
    }
    return true;
}

bool scenePlateParticipatesInPreview(const ScenePlate& plate, int timelineFrame) noexcept
{
    return scenePlateVisibleAtTimelineFrame(plate, timelineFrame)
        && plate.outputMode != ScenePlateOutputMode::ReferenceOnly;
}

bool scenePlateParticipatesInRenderOutput(const ScenePlate& plate, int timelineFrame) noexcept
{
    return scenePlateVisibleAtTimelineFrame(plate, timelineFrame)
        && plate.outputMode == ScenePlateOutputMode::RenderOutput;
}

} // namespace perapera
