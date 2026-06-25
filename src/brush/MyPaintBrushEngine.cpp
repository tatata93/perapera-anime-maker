// このファイルの役割:
// libmypaintを使うブラシエンジンを実装する。
// Phase 1.5 Step 14cでは、確定時一括処理をやめ、ドラッグ中に1点ずつlibmypaintへ投入する。
// Phase 1.5 Step 14dでは、前方宣言MyPaintStateのunique_ptr破棄をcustom deleter化してMSVC C4150を避ける。
// これにより、ペンを離した瞬間の重い一括dab生成をなくす。

#include "brush/MyPaintBrushEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

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

    // CPU CanvasBitmapで毎dabの周辺サンプリングを行うと重くなるため、
    // Step 14cでは逐次処理化後もsmudge/水彩混色はまだ無効化しておく。
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
    surface.dabCallCount = 0;
    surface.visibleDabCount = 0;
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

void configureBrush(MyPaintBrush* brush, const Stroke& stroke, float opacity)
{
    mypaint_brush_from_defaults(brush);

    const float radiusLog = std::log(std::max(0.5f, stroke.radiusPx));
    const auto hsv = rgbToHsv(stroke.color[0], stroke.color[1], stroke.color[2]);
    const float strokeOpacity = clamp01(stroke.color[3] * opacity);
    const float hardness = clamp01(stroke.hardness);
    const float spacing = std::clamp(stroke.spacing, 0.05f, 1.0f);
    const float pressureToSize = clamp01(stroke.pressureToSize);
    const float pressureToOpacity = clamp01(stroke.pressureToOpacity);

    // Step 14c:
    // 確定時一括処理ではなく逐次処理にしたため、長いストロークを丸ごと後処理しない。
    // ただし毎フレームのdab過密化は避けるため、密度は控えめに制限する。
    const float dabDensity = std::clamp(1.0f / std::max(0.12f, spacing), 1.0f, 4.0f);

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
    setBaseValue(brush, "smudge", 0.0f);
    setBaseValue(brush, "smudge_length", 0.0f);
    setBaseValue(brush, "colorize", 0.0f);
    setBaseValue(brush, "lock_alpha", 0.0f);
    setBaseValue(brush, "color_h", hsv[0]);
    setBaseValue(brush, "color_s", hsv[1]);
    setBaseValue(brush, "color_v", hsv[2]);

    setPressureMapping(brush, "radius_logarithmic", radiusLog - 0.9f * pressureToSize, radiusLog);
    setPressureMapping(brush, "opaque", strokeOpacity * lerp(1.0f, 0.25f, pressureToOpacity), strokeOpacity);
}

#endif // PERAPERA_HAS_LIBMYPAINT

} // namespace

#ifdef PERAPERA_HAS_LIBMYPAINT
struct MyPaintBrushEngine::MyPaintState {
    MyPaintBrush* brush = nullptr;
    PeraperaMyPaintSurface surface{};

    ~MyPaintState()
    {
        if (brush != nullptr) {
            mypaint_brush_unref(brush);
            brush = nullptr;
        }
    }
};

void MyPaintBrushEngine::MyPaintStateDeleter::operator()(MyPaintState* state) const noexcept
{
    delete state;
}
#endif

MyPaintBrushEngine::~MyPaintBrushEngine()
{
    endStroke();
}

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

void MyPaintBrushEngine::beginStroke(CanvasBitmap& canvas, const Stroke& strokeSettings, float opacity)
{
#ifndef PERAPERA_HAS_LIBMYPAINT
    (void)canvas;
    (void)strokeSettings;
    (void)opacity;
#else
    endStroke();

    MyPaintBrush* brush = mypaint_brush_new();
    if (brush == nullptr) {
        return;
    }

    configureBrush(brush, strokeSettings, opacity);
    mypaint_brush_new_stroke(brush);

    // state_ は custom deleter 付き unique_ptr なので、std::make_unique の
    // default_delete 付き unique_ptr は代入できない。
    // ここでは reset(pointer) を使い、破棄は MyPaintStateDeleter に任せる。
    state_.reset(new MyPaintState());
    state_->brush = brush;

    // configureBrush内でstroke色アルファとopacityをopaqueへ入れているため、
    // Surface側でさらにopacityを二重掛けしない。
    initSurface(state_->surface, canvas, 1.0f);
#endif
}

bool MyPaintBrushEngine::addPoint(const StrokePoint& point, float deltaTimeSec)
{
#ifndef PERAPERA_HAS_LIBMYPAINT
    (void)point;
    (void)deltaTimeSec;
    return false;
#else
    if (!state_ || state_->brush == nullptr || state_->surface.canvas == nullptr) {
        return false;
    }

    const int beforeVisibleDabs = state_->surface.visibleDabCount;
    const double dtime = std::clamp(static_cast<double>(deltaTimeSec), 0.006, 0.080);
    mypaint_brush_stroke_to(state_->brush,
                            &state_->surface.parent,
                            point.x,
                            point.y,
                            clamp01(point.pressure),
                            0.0f,
                            0.0f,
                            dtime);
    return state_->surface.visibleDabCount > beforeVisibleDabs;
#endif
}

void MyPaintBrushEngine::endStroke()
{
#ifdef PERAPERA_HAS_LIBMYPAINT
    state_.reset();
#endif
}

void MyPaintBrushEngine::bakeStroke(CanvasBitmap& canvas, const Stroke& stroke, float opacity)
{
    // Step 14c:
    // libmypaintはドラッグ中逐次処理で使う。
    // 保存データ再構築・libmypaint未使用時・読み込み時の再ベイクでは、重い一括処理を避けてSimple互換にする。
    canvas.bakeStroke(stroke, opacity);
}

void MyPaintBrushEngine::eraseCircle(CanvasBitmap& canvas, float cx, float cy, float radius)
{
    canvas.eraseCircle(cx, cy, radius);
}

} // namespace perapera
