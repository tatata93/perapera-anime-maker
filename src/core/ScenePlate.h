// このファイルの役割:
// 絵コンテ、レイアウト、仮背景、完成背景などをセル列とは別に扱う
// Scene Plate の最小データモデルを定義する。
// UI、保存形式、描画処理へは依存しない。

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace perapera {

// Scene Plate の制作上の種類。
// 出力可否は kind ではなく outputMode で決める。
enum class ScenePlateKind {
    Storyboard,        // 絵コンテ。参照用。
    Layout,            // レイアウト、背景原図など。
    ReferenceImage,    // 資料画像。
    TemporaryBackground, // 仮背景。
    FinalBackground,   // 完成背景。
};

// Scene Plate をどこまで使うか。
enum class ScenePlateOutputMode {
    ReferenceOnly, // 作画参照だけ。PNG/MP4へは出さない。
    PreviewOnly,   // アプリ内プレビューだけ。正式出力へは出さない。
    RenderOutput,  // 正式出力へ出す。
};

const char* scenePlateKindToString(ScenePlateKind kind) noexcept;
ScenePlateKind scenePlateKindFromString(std::string_view value) noexcept;
const char* scenePlateKindJapaneseLabel(ScenePlateKind kind) noexcept;

const char* scenePlateOutputModeToString(ScenePlateOutputMode mode) noexcept;
ScenePlateOutputMode scenePlateOutputModeFromString(std::string_view value) noexcept;
const char* scenePlateOutputModeJapaneseLabel(ScenePlateOutputMode mode) noexcept;

// キャンバス上での配置。
// rotationDegrees は時計回り正方向として扱う想定。
struct ScenePlateTransform {
    float x = 0.0f;
    float y = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float rotationDegrees = 0.0f;
};

struct ScenePlate {
    std::string id;
    std::string displayName;
    ScenePlateKind kind = ScenePlateKind::Storyboard;
    ScenePlateOutputMode outputMode = ScenePlateOutputMode::ReferenceOnly;

    // 将来の画像読み込みへ接続するパス。
    // Step 7.11ではまだ読み込まない。
    std::string imagePath;

    bool visible = true;
    bool locked = false;
    float opacity = 0.5f;
    int zOrder = 0;

    // 1始まりTの表示範囲。0以下なら範囲制限なし。
    int startTimelineFrame = 0;
    int endTimelineFrame = 0;

    ScenePlateTransform transform;
};

struct ScenePlateStack {
    std::vector<ScenePlate> plates;
};

float clampScenePlateOpacity(float opacity) noexcept;
void normalizeScenePlate(ScenePlate& plate);
void normalizeScenePlateStack(ScenePlateStack& stack);

bool scenePlateVisibleAtTimelineFrame(const ScenePlate& plate, int timelineFrame) noexcept;
bool scenePlateParticipatesInPreview(const ScenePlate& plate, int timelineFrame) noexcept;
bool scenePlateParticipatesInRenderOutput(const ScenePlate& plate, int timelineFrame) noexcept;

} // namespace perapera
