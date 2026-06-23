// このファイルの役割:
// PNG書き出しを実装する。
// 外部ライブラリ追加を避けるため、zlibの非圧縮deflateブロックを使う最小PNGエンコーダを内蔵する。

#include "io/PngExporter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace perapera {
namespace {

struct ImageRgba {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;
};

std::uint8_t toByte(float value)
{
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

void blendPixel(ImageRgba& image, int x, int y, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    if (x < 0 || y < 0 || x >= image.width || y >= image.height || a == 0U) {
        return;
    }

    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                                static_cast<std::size_t>(x)) * 4U;
    const float srcA = static_cast<float>(a) / 255.0f;
    const float dstA = static_cast<float>(image.pixels[offset + 3U]) / 255.0f;
    const float outA = srcA + dstA * (1.0f - srcA);
    if (outA <= 0.0f) {
        return;
    }

    const float srcR = static_cast<float>(r) / 255.0f;
    const float srcG = static_cast<float>(g) / 255.0f;
    const float srcB = static_cast<float>(b) / 255.0f;
    const float dstR = static_cast<float>(image.pixels[offset + 0U]) / 255.0f;
    const float dstG = static_cast<float>(image.pixels[offset + 1U]) / 255.0f;
    const float dstB = static_cast<float>(image.pixels[offset + 2U]) / 255.0f;

    image.pixels[offset + 0U] = toByte((srcR * srcA + dstR * dstA * (1.0f - srcA)) / outA);
    image.pixels[offset + 1U] = toByte((srcG * srcA + dstG * dstA * (1.0f - srcA)) / outA);
    image.pixels[offset + 2U] = toByte((srcB * srcA + dstB * dstA * (1.0f - srcA)) / outA);
    image.pixels[offset + 3U] = toByte(outA);
}

void stampCircle(ImageRgba& image, float cx, float cy, float radius,
                 std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    const int x0 = std::max(0, static_cast<int>(std::floor(cx - radius - 1.0f)));
    const int y0 = std::max(0, static_cast<int>(std::floor(cy - radius - 1.0f)));
    const int x1 = std::min(image.width, static_cast<int>(std::ceil(cx + radius + 1.0f)));
    const int y1 = std::min(image.height, static_cast<int>(std::ceil(cy + radius + 1.0f)));

    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - cx;
            const float dy = static_cast<float>(y) + 0.5f - cy;
            const float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius + 0.5f) {
                continue;
            }
            const float coverage = std::clamp(radius + 0.5f - dist, 0.0f, 1.0f);
            blendPixel(image, x, y, r, g, b, static_cast<std::uint8_t>(std::lround(static_cast<float>(a) * coverage)));
        }
    }
}

void rasterizeStroke(ImageRgba& image, const Stroke& stroke, float opacity)
{
    if (stroke.points.empty()) {
        return;
    }

    const std::uint8_t r = toByte(stroke.color[0]);
    const std::uint8_t g = toByte(stroke.color[1]);
    const std::uint8_t b = toByte(stroke.color[2]);
    const std::uint8_t a = toByte(stroke.color[3] * opacity);
    const float baseRadius = std::max(0.5f, stroke.radiusPx);
    const float spacing = std::max(0.5f, baseRadius * 0.5f);

    if (stroke.points.size() == 1U) {
        const StrokePoint& point = stroke.points.front();
        stampCircle(image, point.x, point.y, baseRadius * std::clamp(point.pressure, 0.0f, 1.0f), r, g, b, a);
        return;
    }

    for (std::size_t index = 1; index < stroke.points.size(); ++index) {
        const StrokePoint& p0 = stroke.points[index - 1U];
        const StrokePoint& p1 = stroke.points[index];
        const float dx = p1.x - p0.x;
        const float dy = p1.y - p0.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        const int steps = std::max(1, static_cast<int>(std::ceil(length / spacing)));
        for (int step = 0; step <= steps; ++step) {
            const float t = static_cast<float>(step) / static_cast<float>(steps);
            const float pressure = p0.pressure + (p1.pressure - p0.pressure) * t;
            stampCircle(image, p0.x + dx * t, p0.y + dy * t, baseRadius * std::clamp(pressure, 0.0f, 1.0f), r, g, b, a);
        }
    }
}

ImageRgba rasterizeFrame(const Frame& frame, int width, int height)
{
    ImageRgba image;
    image.width = std::max(1, width);
    image.height = std::max(1, height);
    image.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4U, 0U);

    for (const Layer& layer : frame.layers) {
        if (!layer.visible || layer.opacity <= 0.0f) {
            continue;
        }
        for (const Stroke& stroke : layer.strokes) {
            rasterizeStroke(image, stroke, layer.opacity);
        }
    }
    return image;
}

std::uint32_t crc32(const std::uint8_t* data, std::size_t size)
{
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1U) ^ (0xedb88320U & (0U - (crc & 1U)));
        }
    }
    return crc ^ 0xffffffffU;
}

std::uint32_t adler32(const std::vector<std::uint8_t>& data)
{
    constexpr std::uint32_t mod = 65521U;
    std::uint32_t a = 1U;
    std::uint32_t b = 0U;
    for (std::uint8_t value : data) {
        a = (a + value) % mod;
        b = (b + a) % mod;
    }
    return (b << 16U) | a;
}

void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    out.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void appendChunk(std::vector<std::uint8_t>& png, const char type[4], const std::vector<std::uint8_t>& data)
{
    appendU32(png, static_cast<std::uint32_t>(data.size()));
    const std::size_t typeOffset = png.size();
    png.insert(png.end(), type, type + 4);
    png.insert(png.end(), data.begin(), data.end());
    const std::uint32_t crc = crc32(png.data() + typeOffset, png.size() - typeOffset);
    appendU32(png, crc);
}

std::vector<std::uint8_t> zlibStore(const std::vector<std::uint8_t>& raw)
{
    std::vector<std::uint8_t> out;
    out.push_back(0x78U);
    out.push_back(0x01U);

    std::size_t offset = 0;
    while (offset < raw.size()) {
        const std::size_t remaining = raw.size() - offset;
        const std::uint16_t blockSize = static_cast<std::uint16_t>(std::min<std::size_t>(remaining, 65535U));
        const bool finalBlock = offset + blockSize >= raw.size();
        out.push_back(finalBlock ? 1U : 0U);
        out.push_back(static_cast<std::uint8_t>(blockSize & 0xffU));
        out.push_back(static_cast<std::uint8_t>((blockSize >> 8U) & 0xffU));
        const std::uint16_t inverse = static_cast<std::uint16_t>(~blockSize);
        out.push_back(static_cast<std::uint8_t>(inverse & 0xffU));
        out.push_back(static_cast<std::uint8_t>((inverse >> 8U) & 0xffU));
        out.insert(out.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset), raw.begin() + static_cast<std::ptrdiff_t>(offset + blockSize));
        offset += blockSize;
    }

    appendU32(out, adler32(raw));
    return out;
}

bool writePng(const ImageRgba& image, const std::filesystem::path& outputPath, std::string* errorMessage)
{
    std::error_code errorCode;
    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path(), errorCode);
        if (errorCode) {
            setError(errorMessage, errorCode.message());
            return false;
        }
    }

    std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<std::size_t>(image.height) * (static_cast<std::size_t>(image.width) * 4U + 1U));
    for (int y = 0; y < image.height; ++y) {
        raw.push_back(0U);
        const std::size_t rowOffset = static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) * 4U;
        raw.insert(raw.end(), image.pixels.begin() + static_cast<std::ptrdiff_t>(rowOffset),
                   image.pixels.begin() + static_cast<std::ptrdiff_t>(rowOffset + static_cast<std::size_t>(image.width) * 4U));
    }

    std::vector<std::uint8_t> png = {0x89U, 'P', 'N', 'G', 0x0dU, 0x0aU, 0x1aU, 0x0aU};
    std::vector<std::uint8_t> ihdr;
    appendU32(ihdr, static_cast<std::uint32_t>(image.width));
    appendU32(ihdr, static_cast<std::uint32_t>(image.height));
    ihdr.push_back(8U);
    ihdr.push_back(6U);
    ihdr.push_back(0U);
    ihdr.push_back(0U);
    ihdr.push_back(0U);
    appendChunk(png, "IHDR", ihdr);
    appendChunk(png, "IDAT", zlibStore(raw));
    appendChunk(png, "IEND", {});

    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        setError(errorMessage, "failed to open output file");
        return false;
    }
    file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return true;
}

std::filesystem::path sequencePath(const std::filesystem::path& folder, int index)
{
    std::ostringstream name;
    name << "frame_" << std::setw(3) << std::setfill('0') << index << ".png";
    return folder / name.str();
}

} // namespace

bool PngExporter::exportFrame(const Frame& frame,
                              const std::filesystem::path& outputPath,
                              int width,
                              int height,
                              std::string* errorMessage)
{
    return writePng(rasterizeFrame(frame, width, height), outputPath, errorMessage);
}

bool PngExporter::exportFrameSequence(const Cell& cell,
                                      const std::filesystem::path& outputFolder,
                                      int width,
                                      int height,
                                      std::string* errorMessage)
{
    if (cell.frames.empty()) {
        setError(errorMessage, "cell has no frames");
        return false;
    }

    for (std::size_t index = 0; index < cell.frames.size(); ++index) {
        const std::filesystem::path outputPath = sequencePath(outputFolder, static_cast<int>(index) + 1);
        if (!exportFrame(cell.frames[index], outputPath, width, height, errorMessage)) {
            return false;
        }
    }
    return true;
}

} // namespace perapera
