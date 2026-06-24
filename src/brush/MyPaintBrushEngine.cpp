// このファイルの役割:
// libmypaintを使うブラシエンジンを実装する。
// Phase 1.5 Step 13では、MyPaintSurface::draw_dab を CanvasBitmap::paintDab へ接続する。
// libmypaintが見つからない環境では、従来のSimple互換へ安全にフォールバックする。
// Step 13dでは、交差部分が欠けるのを避けるため、alpha_eraserを無視し、細いSimple連続芯を後段で足す。
// Step 14では、Strokeに保存された硬さ・間隔・筆圧・水彩系パラメータをlibmypaint設定へ反映する。
// Step 14bでは、1ストローク確定時に固まる問題を避けるため、重いsmudge/過密dabを抑制し、
// 長いストロークはSimple互換へ安全に退避する。

#include "brush/MyPaintBrushEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>

#include "render/CanvasBitmap.h"

#ifdef PERAPERA_HAS_LIBMYPAINT
extern "C" {
#include <mypaint-brush.h>
#include <mypaint-brush-settings.h>
#include <mypaint-surface.h>
}
#endif

namespace perapera {
namespace {

float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * clamp01(t);
}

float distanceBetween(const StrokePoint& a, const StrokePoint& b)
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

float strokePathLength(const Stroke& stroke)
{
    if (stroke.points.size() <= 1U) {
        return 0.0f;
    }

    float length = 0.0f;
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        length += distanceBetween(stroke.points[index - 1U], stroke.points[index]);
    }
    return length;
}

bool shouldUseFastSimpleFallback(const Stroke& stroke)
{
    if (stroke.points.size() <= 1U) {
        return true;
    }

    // Step 14b:
    // libmypaintはストローク確定時に多数のdabを生成する。
    // 大きい半径・長い線・細かいspacingが重なると、1ストローク確定でUIが止まる。
    // ここでは確定時フリーズを避けるため、重すぎる入力だけSimple互換へ退避する。
    const float lengthPx = strokePathLength(stroke);
    const float radius = std::max(0.5f, stroke.radiusPx);
    const float spacing = std::clamp(stroke.spacing, 0.08f, 1.0f);
    const float estimatedDabs = lengthPx / std::max(1.0f, radius * spacing);
    const float estimatedPixelVisits = estimatedDabs * radius * radius;

    if (stroke.points.size() > 420U) {
        return true;
    }
    if (lengthPx > 3600.0f) {
        return true;
    }
    if (estimatedPixelVisits > 180000.0f) {
        return true;
    }
    if (radius > 24.0f && lengthPx > 800.0f) {
        return true;
    }
    return false;
}

std::array<float, 3> rgbToHsv(float r, float g, float b)
{
    r = clamp01(r);
    g = clamp01(g);
    b = clamp01(b);

    const float maxValue = std::max({r, g, b});
    const float minValue = std::min({r, g, b});
    const float delta = maxValue - minValue;

    float h = 0.0f;
    if (delta > 0.00001f) {
        if (maxValue == r) {
            h = std::fmod((g - b) / delta, 6.0f);
        } else if (maxValue == g) {
            h = ((b - r) / delta) + 2.0f;
        } else {
            h = ((r - g) / delta) + 4.0f;
        }
        h /= 6.0f;
        if (h < 0.0f) {
            h += 1.0f;
        }
    }

    const float s = maxValue <= 0.00001f ? 0.0f : delta / maxValue;
    return {clamp01(h), clamp01(s), clamp01(maxValue)};
}

#ifdef PERAPERA_HAS_LIBMYPAINT

struct PeraperaMyPaintSurface {
    MyPaintSurface parent{};
    CanvasBitmap* canvas = nullptr;
    float strokeOpacity = 1.0f;
    int dabCallCount = 0;
    int visibleDabCount = 0;
};

PeraperaMyPaintSurface* toPeraperaSurface(MyPaintSurface* surface)
{
    return reinterpret_cast<PeraperaMyPaintSurface*>(surface);
}

int surfaceDrawDab(MyPaintSurface* self,
                   float x,
                   float y,
                   float radius,
                   float colorR,
                   float colorG,
                   float colorB,
                   float opaque,
                   float hardness,
                   float alphaEraser,
                   float aspectRatio,
                   float angle,
                   float lockAlpha,
                   float colorize)
{
    (void)alphaEraser;
    (void)aspectRatio;
    (void)angle;
    (void)lockAlpha;
    (void)colorize;

    PeraperaMyPaintSurface* surface = toPeraperaSurface(self);
    if (surface == nullptr || surface->canvas == nullptr) {
        return 0;
    }

    ++surface->dabCallCount;

    const float opacity = clamp01(opaque * surface->strokeOpacity);
    const float safeRadius = std::max(0.5f, radius);
    surface->canvas->paintDab(x,
                              y,
                              safeRadius,
                              colorR,
                              colorG,
                              colorB,
                              opacity,
                              hardness);
    if (opacity > (1.0f / 255.0f) && safeRadius > 0.0f) {
        ++surface->visibleDabCount;
    }
    return 1;
}

void surfaceGetColor(MyPaintSurface* self,
                     float x,
                     float y,
                     float radius,
                     float* colorR,
                     float* colorG,
                     float* colorB,
                     float* colorA)
{
    (void)self;
    (void)x;
    (void)y;
    (void)radius;

    // Step 14b:
    // get_colorで毎dab周辺ピクセルを走査すると、長いストローク確定時の停止感が大きい。
    // 本格smudge/水彩混色は後続Stepに回し、現段階では透明色を返して重いサンプリングを止める。
    if (colorR != nullptr) {
        *colorR = 0.0f;
    }
    if (colorG != nullptr) {
        *colorG = 0.0f;
    }
    if (colorB != nullptr) {
        *colorB = 0.0f;
    }
    if (colorA != nullptr) {
        *colorA = 0.0f;
    }
}

void surfaceBeginAtomic(MyPaintSurface* self)
{
    (void)self;
}

void surfaceEndAtomic(MyPaintSurface* self, MyPaintRectangle* roi)
{
    (void)self;
    (void)roi;
}

void surfaceDestroy(MyPaintSurface* self)
{
    (void)self;
}

void surfaceSavePng(MyPaintSurface* self, const char* path, int x, int y, int width, int height)
{
    (void)self;
    (void)path;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void initSurface(PeraperaMyPaintSurface& surface, CanvasBitmap& canvas, float strokeOpacity)
{
    mypaint_surface_init(&surface.parent);
    surface.canvas = &canvas;
    surface.strokeOpacity = clamp01(strokeOpacity);
    surface.parent.draw_dab = surfaceDrawDab;
    surface.parent.get_color = surfaceGetColor;
    surface.parent.begin_atomic = surfaceBeginAtomic;
    surface.parent.end_atomic = surfaceEndAtomic;
    surface.parent.destroy = surfaceDestroy;
    surface.parent.save_png = surfaceSavePng;
}

void setBaseValue(MyPaintBrush* brush, const char* cname, float value)
{
    const MyPaintBrushSetting setting = mypaint_brush_setting_from_cname(cname);
    if (static_cast<int>(setting) >= 0) {
        mypaint_brush_set_base_value(brush, setting, value);
    }
}

void setPressureMapping(MyPaintBrush* brush,
                        const char* settingName,
                        float lowValue,
                        float highValue)
{
    const MyPaintBrushSetting setting = mypaint_brush_setting_from_cname(settingName);
    const MyPaintBrushInput pressure = mypaint_brush_input_from_cname("pressure");
    if (static_cast<int>(setting) < 0 || static_cast<int>(pressure) < 0) {
        return;
    }

    mypaint_brush_set_mapping_n(brush, setting, pressure, 2);
    mypaint_brush_set_mapping_point(brush, setting, pressure, 0, 0.0f, lowValue);
    mypaint_brush_set_mapping_point(brush, setting, pressure, 1, 1.0f, highValue);
}

void bakeSimpleContinuityCore(CanvasBitmap& canvas, const Stroke& stroke, float opacity)
{
    if (stroke.points.empty()) {
        return;
    }

    Stroke continuityStroke = stroke;
    const float hardness = clamp01(stroke.hardness);
    const float watercolor = clamp01(stroke.watercolorBleed);
    const float coreScale = lerp(0.24f, 0.42f, hardness) * lerp(1.0f, 0.72f, watercolor);
    continuityStroke.radiusPx = std::max(0.45f, stroke.radiusPx * coreScale);
    continuityStroke.opacity = 1.0f;
    canvas.bakeStroke(continuityStroke, std::clamp(opacity * lerp(0.35f, 0.70f, hardness), 0.0f, 1.0f));
}

void configureBrush(MyPaintBrush* brush, const Stroke& stroke, float opacity)
{
    mypaint_brush_from_defaults(brush);

    // libmypaint の radius_logarithmic は自然対数に近いスケール。
    // 例: log(2)≈0.7, log(20)≈3.0 で公式設定説明と一致する。
    const float radiusLog = std::log(std::max(0.5f, stroke.radiusPx));
    const auto hsv = rgbToHsv(stroke.color[0], stroke.color[1], stroke.color[2]);
    const float strokeOpacity = clamp01(stroke.color[3] * opacity);
    const float hardness = clamp01(stroke.hardness);
    const float spacing = std::clamp(stroke.spacing, 0.05f, 1.0f);
    const float pressureToSize = clamp01(stroke.pressureToSize);
    const float pressureToOpacity = clamp01(stroke.pressureToOpacity);
    const float watercolor = clamp01(stroke.watercolorBleed);
    const float colorMix = clamp01(stroke.colorMix);
    const float dryRate = clamp01(stroke.dryRate);

    // Step 14: UIで見えているブラシ設定をMyPaintへ反映する。
    // Step 14b: spacingをそのまま高密度dabへ変換すると、1ストローク確定時に重くなる。
    // まず応答性を優先し、dab密度を控えめに抑える。
    const float dabDensity = std::clamp(1.0f / std::max(0.12f, spacing), 1.0f, 4.0f);

    // Step 14b:
    // smudge/watercolorはget_colorの周辺サンプリングが必要で、現状のCPU CanvasBitmapでは重い。
    // UI値は保存するが、libmypaint実描画ではいったん無効化して操作停止を防ぐ。
    (void)watercolor;
    (void)colorMix;
    (void)dryRate;
    const float smudgeStrength = 0.0f;
    const float smudgeLength = 0.0f;

    setBaseValue(brush, "radius_logarithmic", radiusLog);
    setBaseValue(brush, "opaque", strokeOpacity);
    setBaseValue(brush, "opaque_multiply", 1.0f);
    setBaseValue(brush, "hardness", hardness);
    setBaseValue(brush, "anti_aliasing", 1.0f);
    setBaseValue(brush, "dabs_per_actual_radius", dabDensity);
    setBaseValue(brush, "dabs_per_basic_radius", dabDensity);
    setBaseValue(brush, "dabs_per_second", 0.0f);
    setBaseValue(brush, "eraser", 0.0f);
    setBaseValue(brush, "paint_mode", 0.0f);
    setBaseValue(brush, "smudge", smudgeStrength);
    setBaseValue(brush, "smudge_length", smudgeLength);
    setBaseValue(brush, "colorize", 0.0f);
    setBaseValue(brush, "lock_alpha", 0.0f);
    setBaseValue(brush, "color_h", hsv[0]);
    setBaseValue(brush, "color_s", hsv[1]);
    setBaseValue(brush, "color_v", hsv[2]);

    // 現在のStrokePoint::pressureを活かす筆圧マッピング。
    // マウス入力はpressure=1.0なので既存挙動に近い。
    setPressureMapping(brush, "radius_logarithmic", radiusLog - 0.9f * pressureToSize, radiusLog);
    setPressureMapping(brush, "opaque", strokeOpacity * lerp(1.0f, 0.25f, pressureToOpacity), strokeOpacity);
}

#endif // PERAPERA_HAS_LIBMYPAINT

} // namespace

bool MyPaintBrushEngine::isLibraryAvailable() const
{
#ifdef PERAPERA_HAS_LIBMYPAINT
    return true;
#else
    return false;
#endif
}

const char* MyPaintBrushEngine::backendName() const
{
    return isLibraryAvailable() ? "libmypaint active" : "simple fallback";
}

void MyPaintBrushEngine::bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity)
{
#ifndef PERAPERA_HAS_LIBMYPAINT
    canvas.bakeStroke(stroke, opacity);
#else
    if (stroke.points.empty()) {
        return;
    }
    if (shouldUseFastSimpleFallback(stroke)) {
        // Step 14b:
        // 重すぎるストロークは、確定時にUIを止めるよりSimple互換で即時確定する。
        // 保存上のbrushEngineはMyPaintのままなので、後続Stepで再レンダリング品質を改善できる。
        canvas.bakeStroke(stroke, opacity);
        return;
    }

    MyPaintBrush* brush = mypaint_brush_new();
    if (brush == nullptr) {
        canvas.bakeStroke(stroke, opacity);
        return;
    }

    configureBrush(brush, stroke, opacity);
    mypaint_brush_new_stroke(brush);

    PeraperaMyPaintSurface surface;
    initSurface(surface, canvas, opacity);

    const StrokePoint& first = stroke.points.front();
    mypaint_brush_stroke_to(brush,
                            &surface.parent,
                            first.x,
                            first.y,
                            clamp01(first.pressure),
                            0.0f,
                            0.0f,
                            0.01);

    StrokePoint previousSubmitted = first;
    const float inputStepPx = std::max(1.0f, stroke.radiusPx * 0.18f);
    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& current = stroke.points[index];
        const bool isLastPoint = index + 1U >= stroke.points.size();
        const float distancePx = distanceBetween(previousSubmitted, current);
        if (!isLastPoint && distancePx < inputStepPx) {
            continue;
        }

        const double dtime = std::clamp(static_cast<double>(distancePx) / 180.0, 0.006, 0.080);

        mypaint_brush_stroke_to(brush,
                                &surface.parent,
                                current.x,
                                current.y,
                                clamp01(current.pressure),
                                0.0f,
                                0.0f,
                                dtime);
        previousSubmitted = current;
    }

    const bool shouldFallbackToSimple = surface.visibleDabCount <= 0;
    mypaint_brush_unref(brush);

    if (shouldFallbackToSimple) {
        // libmypaintが検出されても、設定値・入力点列・環境差でdabが1つも出ない場合がある。
        // その場合、ドラッグ中のDrawListプレビューだけ見えて、確定後に線が消える。
        // ここでは描画結果の消失を防ぐため、保存形式はMyPaintのまま、ピクセル焼き込みだけSimpleへ退避する。
        canvas.bakeStroke(stroke, opacity);
        return;
    }

    // libmypaintのdabだけだと、ストローク同士が交差した箇所や急な方向転換で、
    // 既存ピクセルとの相互作用により細い欠けが見えることがある。
    // 確定後に線が遮られたように見えるのを防ぐため、描画確定時だけ細いSimple芯を重ねる。
    // 毎フレーム処理ではなくストローク確定時のみなので、60fps方針には影響させない。
    bakeSimpleContinuityCore(canvas, stroke, opacity);
#endif
}

void MyPaintBrushEngine::eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius)
{
    canvas.eraseCircle(cx, cy, radius);
}

} // namespace perapera
