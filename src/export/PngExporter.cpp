// src/export/PngExporter.cpp
//
// PNG保存処理の実装です。
//
// Phase 3Cでは、まだ高品質なアンチエイリアスは行いません。
// Strokeの線分に沿って円を並べる簡易ラスタライズでPNG化します。

#include "export/PngExporter.h"

#include "drawing/Stroke.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace perapera
{
    namespace
    {
        struct PixelRgba8
        {
            std::uint8_t r = 0;
            std::uint8_t g = 0;
            std::uint8_t b = 0;
            std::uint8_t a = 0;
        };

        struct ImageRgba8
        {
            int width = 0;
            int height = 0;
            std::vector<PixelRgba8> pixels;

            PixelRgba8& pixelAt(int x, int y)
            {
                return pixels[static_cast<std::size_t>(y * width + x)];
            }
        };

        float clamp01(float value)
        {
            return std::clamp(value, 0.0f, 1.0f);
        }

        std::uint8_t toByte(float value)
        {
            return static_cast<std::uint8_t>(clamp01(value) * 255.0f + 0.5f);
        }

        PixelRgba8 makePixel(ColorRgba color, float layerOpacity)
        {
            PixelRgba8 pixel;
            pixel.r = toByte(color.r);
            pixel.g = toByte(color.g);
            pixel.b = toByte(color.b);
            pixel.a = toByte(color.a * layerOpacity);
            return pixel;
        }

        void alphaBlendPixel(ImageRgba8& image, int x, int y, PixelRgba8 source)
        {
            if (x < 0 || y < 0 || x >= image.width || y >= image.height)
            {
                return;
            }

            PixelRgba8& destination = image.pixelAt(x, y);

            const float srcA = static_cast<float>(source.a) / 255.0f;
            const float dstA = static_cast<float>(destination.a) / 255.0f;

            const float outA = srcA + dstA * (1.0f - srcA);

            if (outA <= 0.0f)
            {
                destination = PixelRgba8{};
                return;
            }

            const float srcR = static_cast<float>(source.r) / 255.0f;
            const float srcG = static_cast<float>(source.g) / 255.0f;
            const float srcB = static_cast<float>(source.b) / 255.0f;

            const float dstR = static_cast<float>(destination.r) / 255.0f;
            const float dstG = static_cast<float>(destination.g) / 255.0f;
            const float dstB = static_cast<float>(destination.b) / 255.0f;

            const float outR = (srcR * srcA + dstR * dstA * (1.0f - srcA)) / outA;
            const float outG = (srcG * srcA + dstG * dstA * (1.0f - srcA)) / outA;
            const float outB = (srcB * srcA + dstB * dstA * (1.0f - srcA)) / outA;

            destination.r = toByte(outR);
            destination.g = toByte(outG);
            destination.b = toByte(outB);
            destination.a = toByte(outA);
        }

        void drawFilledCircle(
            ImageRgba8& image,
            float centerX,
            float centerY,
            float radius,
            PixelRgba8 color
        )
        {
            const int minX = static_cast<int>(std::floor(centerX - radius));
            const int maxX = static_cast<int>(std::ceil(centerX + radius));
            const int minY = static_cast<int>(std::floor(centerY - radius));
            const int maxY = static_cast<int>(std::ceil(centerY + radius));

            const float radiusSquared = radius * radius;

            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    const float dx = (static_cast<float>(x) + 0.5f) - centerX;
                    const float dy = (static_cast<float>(y) + 0.5f) - centerY;

                    if (dx * dx + dy * dy <= radiusSquared)
                    {
                        alphaBlendPixel(image, x, y, color);
                    }
                }
            }
        }

        void drawStrokeToImage(
            ImageRgba8& image,
            const Stroke& stroke,
            float cropMinX,
            float cropMinY,
            float layerOpacity
        )
        {
            if (!stroke.hasDrawablePoints())
            {
                return;
            }

            const PixelRgba8 color = makePixel(stroke.color, layerOpacity);
            const float radius = std::max(1.0f, stroke.radiusPx);

            auto toImageX = [cropMinX](float canvasX)
            {
                return canvasX - cropMinX;
            };

            auto toImageY = [cropMinY](float canvasY)
            {
                return canvasY - cropMinY;
            };

            if (stroke.points.size() == 1)
            {
                const CanvasPoint point = stroke.points.front();

                drawFilledCircle(
                    image,
                    toImageX(point.x),
                    toImageY(point.y),
                    radius,
                    color
                );

                return;
            }

            for (std::size_t i = 1; i < stroke.points.size(); ++i)
            {
                const CanvasPoint a = stroke.points[i - 1];
                const CanvasPoint b = stroke.points[i];

                const float ax = toImageX(a.x);
                const float ay = toImageY(a.y);
                const float bx = toImageX(b.x);
                const float by = toImageY(b.y);

                const float dx = bx - ax;
                const float dy = by - ay;
                const float length = std::sqrt(dx * dx + dy * dy);

                if (length <= 0.0001f)
                {
                    drawFilledCircle(image, ax, ay, radius, color);
                    continue;
                }

                const float step = std::max(1.0f, radius * 0.5f);
                const int count = std::max(1, static_cast<int>(std::ceil(length / step)));

                for (int j = 0; j <= count; ++j)
                {
                    const float t = static_cast<float>(j) / static_cast<float>(count);
                    const float x = ax + dx * t;
                    const float y = ay + dy * t;

                    drawFilledCircle(image, x, y, radius, color);
                }
            }
        }

        void fillBackground(ImageRgba8& image, bool transparentBackground)
        {
            PixelRgba8 background;

            if (transparentBackground)
            {
                background = PixelRgba8{0, 0, 0, 0};
            }
            else
            {
                background = PixelRgba8{44, 48, 56, 255};
            }

            std::fill(image.pixels.begin(), image.pixels.end(), background);
        }
    }

    bool PngExporter::exportCenteredRenderFrame(
        const std::filesystem::path& outputPath,
        const WorkCanvas& workCanvas,
        const RenderFormat& renderFormat,
        const std::vector<DrawingLayer>& layers,
        const PngExportOptions& options,
        std::string& errorMessage
    )
    {
        errorMessage.clear();

        if (renderFormat.outputWidthPx <= 0 || renderFormat.outputHeightPx <= 0)
        {
            errorMessage = "出力サイズが不正です。";
            return false;
        }

        ImageRgba8 image;
        image.width = renderFormat.outputWidthPx;
        image.height = renderFormat.outputHeightPx;
        image.pixels.resize(static_cast<std::size_t>(image.width * image.height));

        fillBackground(image, options.transparentBackground);

        const float cropMinX =
            (static_cast<float>(workCanvas.widthPx) -
             static_cast<float>(renderFormat.outputWidthPx)) * 0.5f;

        const float cropMinY =
            (static_cast<float>(workCanvas.heightPx) -
             static_cast<float>(renderFormat.outputHeightPx)) * 0.5f;

        for (const DrawingLayer& layer : layers)
        {
            if (!layer.visible)
            {
                continue;
            }

            const float layerOpacity = clamp01(layer.opacity);

            for (const Stroke& stroke : layer.strokes)
            {
                drawStrokeToImage(image, stroke, cropMinX, cropMinY, layerOpacity);
            }
        }

        std::error_code errorCode;
        std::filesystem::create_directories(outputPath.parent_path(), errorCode);

        if (errorCode)
        {
            errorMessage = "保存先フォルダを作成できませんでした: " + errorCode.message();
            return false;
        }

        const int strideBytes = image.width * 4;

        const int result = stbi_write_png(
            outputPath.string().c_str(),
            image.width,
            image.height,
            4,
            image.pixels.data(),
            strideBytes
        );

        if (result == 0)
        {
            errorMessage = "PNG書き出しに失敗しました。";
            return false;
        }

        return true;
    }
}