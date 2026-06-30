// このファイルの役割:
// Scene Plate 最小モデルが、参照用/プレビュー用/出力用を安全に分けられることを確認する。

#include "core/ScenePlate.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

namespace {

bool nearlyEqual(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) < 0.0001f;
}

void testStringConversions()
{
    using namespace perapera;

    assert(std::string(scenePlateKindToString(ScenePlateKind::Storyboard)) == "storyboard");
    assert(std::string(scenePlateKindToString(ScenePlateKind::Layout)) == "layout");
    assert(std::string(scenePlateKindToString(ScenePlateKind::ReferenceImage)) == "referenceImage");
    assert(std::string(scenePlateKindToString(ScenePlateKind::TemporaryBackground)) == "temporaryBackground");
    assert(std::string(scenePlateKindToString(ScenePlateKind::FinalBackground)) == "finalBackground");

    assert(scenePlateKindFromString("layout") == ScenePlateKind::Layout);
    assert(scenePlateKindFromString("referenceImage") == ScenePlateKind::ReferenceImage);
    assert(scenePlateKindFromString("temporaryBackground") == ScenePlateKind::TemporaryBackground);
    assert(scenePlateKindFromString("finalBackground") == ScenePlateKind::FinalBackground);
    assert(scenePlateKindFromString("unknown") == ScenePlateKind::Storyboard);

    assert(std::string(scenePlateOutputModeToString(ScenePlateOutputMode::ReferenceOnly)) == "referenceOnly");
    assert(std::string(scenePlateOutputModeToString(ScenePlateOutputMode::PreviewOnly)) == "previewOnly");
    assert(std::string(scenePlateOutputModeToString(ScenePlateOutputMode::RenderOutput)) == "renderOutput");

    assert(scenePlateOutputModeFromString("previewOnly") == ScenePlateOutputMode::PreviewOnly);
    assert(scenePlateOutputModeFromString("renderOutput") == ScenePlateOutputMode::RenderOutput);
    assert(scenePlateOutputModeFromString("unknown") == ScenePlateOutputMode::ReferenceOnly);
}

void testNormalizePlate()
{
    using namespace perapera;

    ScenePlate plate;
    plate.id = "storyboard_a";
    plate.opacity = 2.0f;
    plate.startTimelineFrame = 12;
    plate.endTimelineFrame = 3;
    plate.transform.scaleX = 0.0f;
    plate.transform.scaleY = -2.0f;

    normalizeScenePlate(plate);

    assert(plate.displayName == "storyboard_a");
    assert(nearlyEqual(plate.opacity, 1.0f));
    assert(plate.startTimelineFrame == 3);
    assert(plate.endTimelineFrame == 12);
    assert(nearlyEqual(plate.transform.scaleX, 1.0f));
    assert(nearlyEqual(plate.transform.scaleY, -2.0f));
}

void testStackNormalizeAndOrder()
{
    using namespace perapera;

    ScenePlateStack stack;

    ScenePlate later;
    later.id = "plate";
    later.displayName = "later";
    later.zOrder = 20;
    later.opacity = -1.0f;

    ScenePlate earlier;
    earlier.id = "plate";
    earlier.displayName = "earlier";
    earlier.zOrder = -10;

    ScenePlate unnamed;
    unnamed.zOrder = 0;

    stack.plates.push_back(later);
    stack.plates.push_back(earlier);
    stack.plates.push_back(unnamed);

    normalizeScenePlateStack(stack);

    assert(stack.plates.size() == 3);
    assert(stack.plates[0].displayName == "earlier");
    assert(stack.plates[1].displayName == stack.plates[1].id);
    assert(stack.plates[2].displayName == "later");
    assert(stack.plates[0].id != stack.plates[2].id);
    assert(!stack.plates[1].id.empty());
    assert(nearlyEqual(stack.plates[2].opacity, 0.0f));
}

void testVisibilityAndOutputMode()
{
    using namespace perapera;

    ScenePlate storyboard;
    storyboard.kind = ScenePlateKind::Storyboard;
    storyboard.outputMode = ScenePlateOutputMode::ReferenceOnly;
    storyboard.startTimelineFrame = 2;
    storyboard.endTimelineFrame = 4;
    normalizeScenePlate(storyboard);

    assert(!scenePlateVisibleAtTimelineFrame(storyboard, 1));
    assert(scenePlateVisibleAtTimelineFrame(storyboard, 2));
    assert(scenePlateVisibleAtTimelineFrame(storyboard, 4));
    assert(!scenePlateVisibleAtTimelineFrame(storyboard, 5));
    assert(!scenePlateParticipatesInPreview(storyboard, 3));
    assert(!scenePlateParticipatesInRenderOutput(storyboard, 3));

    ScenePlate preview;
    preview.outputMode = ScenePlateOutputMode::PreviewOnly;
    assert(scenePlateParticipatesInPreview(preview, 0));
    assert(!scenePlateParticipatesInRenderOutput(preview, 0));

    ScenePlate render;
    render.outputMode = ScenePlateOutputMode::RenderOutput;
    assert(scenePlateParticipatesInPreview(render, 0));
    assert(scenePlateParticipatesInRenderOutput(render, 0));

    render.visible = false;
    assert(!scenePlateParticipatesInPreview(render, 0));
    assert(!scenePlateParticipatesInRenderOutput(render, 0));
}

} // namespace

int main()
{
    testStringConversions();
    testNormalizePlate();
    testStackNormalizeAndOrder();
    testVisibilityAndOutputMode();

    std::cout << "Scene plate self-test passed\n";
    return 0;
}
