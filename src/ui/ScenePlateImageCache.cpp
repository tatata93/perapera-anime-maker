// このファイルの役割:
// Scene Plate 用の実画像読み込みとSDL_Textureキャッシュを担当する。
// WindowsではWICでPNG/JPEG/BMP等をRGBAへ変換してSDL_Texture化する。

#include "ui/ScenePlateImageCache.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <wincodec.h>
#include <wrl/client.h>
#endif

namespace perapera {
namespace {

std::string pathKey(const std::filesystem::path& path)
{
    std::error_code ec;
    const std::filesystem::path absolutePath = std::filesystem::absolute(path, ec);
    const std::filesystem::path normalized = (ec ? path : absolutePath).lexically_normal();
    return normalized.string();
}

std::string hresultToString(long hr)
{
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << static_cast<unsigned long>(hr);
    return stream.str();
}

#if defined(_WIN32)
bool win32RegularFileExistsW(const std::wstring& widePath)
{
    if (widePath.empty()) {
        return false;
    }

    const DWORD attributes = GetFileAttributesW(widePath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool win32RegularFileExistsA(const std::string& pathText)
{
    if (pathText.empty()) {
        return false;
    }

    const DWORD attributes = GetFileAttributesA(pathText.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    return (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring widePathFromUtf8OrAnsiText(const std::string& pathText)
{
    if (pathText.empty()) {
        return {};
    }

    auto convert = [&](UINT codePage, DWORD flags) -> std::wstring {
        const int wideLength = MultiByteToWideChar(
            codePage,
            flags,
            pathText.c_str(),
            static_cast<int>(pathText.size()),
            nullptr,
            0);
        if (wideLength <= 0) {
            return {};
        }

        std::wstring widePath(static_cast<std::size_t>(wideLength), L'\0');
        const int convertedLength = MultiByteToWideChar(
            codePage,
            flags,
            pathText.c_str(),
            static_cast<int>(pathText.size()),
            widePath.data(),
            wideLength);
        if (convertedLength <= 0) {
            return {};
        }
        return widePath;
    };

    if (std::wstring utf8Path = convert(CP_UTF8, MB_ERR_INVALID_CHARS); !utf8Path.empty()) {
        return utf8Path;
    }
    return convert(CP_ACP, 0);
}
#endif

bool regularFileExistsForScenePlate(const std::filesystem::path& path)
{
    if (path.empty()) {
        return false;
    }
#if defined(_WIN32)
    const std::wstring widePath = path.wstring();
    if (win32RegularFileExistsW(widePath)) {
        return true;
    }

    const std::string narrowPath = path.string();
    if (win32RegularFileExistsA(narrowPath)) {
        return true;
    }

    if (std::wstring convertedWide = widePathFromUtf8OrAnsiText(narrowPath); !convertedWide.empty()) {
        return win32RegularFileExistsW(convertedWide);
    }
    return false;
#else
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec && std::filesystem::is_regular_file(path, ec) && !ec;
#endif
}

struct LoadedRgbaImage {
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    bool ok = false;
    std::string status;
};

#if defined(_WIN32)
LoadedRgbaImage loadImageWithWic(const std::filesystem::path& imagePath)
{
    using Microsoft::WRL::ComPtr;

    LoadedRgbaImage image;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool needsCoUninitialize = false;
    if (SUCCEEDED(hr)) {
        needsCoUninitialize = true;
    } else if (hr != RPC_E_CHANGED_MODE) {
        image.status = "WIC COM init failed: " + hresultToString(hr);
        return image;
    }

    auto cleanupCom = [&]() {
        if (needsCoUninitialize) {
            CoUninitialize();
        }
    };

    ComPtr<IWICImagingFactory> factory;
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));
    if (FAILED(hr) || factory == nullptr) {
        image.status = "WIC factory failed: " + hresultToString(hr);
        cleanupCom();
        return image;
    }

    const std::wstring widePath = imagePath.wstring();
    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromFilename(
        widePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder);
    if (FAILED(hr) || decoder == nullptr) {
        image.status = "image decode failed: " + hresultToString(hr);
        cleanupCom();
        return image;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || frame == nullptr) {
        image.status = "image frame failed: " + hresultToString(hr);
        cleanupCom();
        return image;
    }

    UINT width = 0;
    UINT height = 0;
    hr = frame->GetSize(&width, &height);
    if (FAILED(hr) || width == 0U || height == 0U) {
        image.status = "image size failed: " + hresultToString(hr);
        cleanupCom();
        return image;
    }

    constexpr UINT kMaxScenePlateImageSize = 16384U;
    if (width > kMaxScenePlateImageSize || height > kMaxScenePlateImageSize) {
        image.status = "image too large";
        cleanupCom();
        return image;
    }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || converter == nullptr) {
        image.status = "image format converter failed: " + hresultToString(hr);
        cleanupCom();
        return image;
    }

    hr = converter->Initialize(
        frame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        image.status = "image RGBA conversion failed: " + hresultToString(hr);
        cleanupCom();
        return image;
    }

    const std::uint64_t stride64 = static_cast<std::uint64_t>(width) * 4ULL;
    const std::uint64_t imageSize64 = stride64 * static_cast<std::uint64_t>(height);
    if (stride64 > static_cast<std::uint64_t>(std::numeric_limits<UINT>::max()) ||
        imageSize64 > static_cast<std::uint64_t>(std::numeric_limits<UINT>::max())) {
        image.status = "image buffer too large";
        cleanupCom();
        return image;
    }

    const UINT stride = static_cast<UINT>(stride64);
    const UINT imageSize = static_cast<UINT>(imageSize64);
    image.pixels.resize(static_cast<std::size_t>(imageSize));
    hr = converter->CopyPixels(nullptr, stride, imageSize, image.pixels.data());
    if (FAILED(hr)) {
        image.pixels.clear();
        image.status = "image pixels failed: " + hresultToString(hr);
        cleanupCom();
        return image;
    }

    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.ok = true;
    image.status = "実画像表示OK";
    cleanupCom();
    return image;
}
#else
LoadedRgbaImage loadImageWithWic(const std::filesystem::path&)
{
    LoadedRgbaImage image;
    image.status = "image loading is currently Windows/WIC only";
    return image;
}
#endif

ScenePlateImageCache::TextureResult makeTransientResult(std::string status)
{
    ScenePlateImageCache::TextureResult result;
    result.attempted = true;
    result.ok = false;
    result.status = std::move(status);
    return result;
}

} // namespace

ScenePlateImageCache::~ScenePlateImageCache()
{
    clear();
}

void ScenePlateImageCache::clear()
{
    for (CacheEntry& entry : entries_) {
        if (entry.result.texture != nullptr) {
            SDL_DestroyTexture(entry.result.texture);
            entry.result.texture = nullptr;
        }
    }
    entries_.clear();
    renderer_ = nullptr;
}

ScenePlateImageCache::CacheEntry* ScenePlateImageCache::findEntry(
    const std::string& key,
    SDL_Renderer* renderer,
    std::uintmax_t fileSize,
    std::filesystem::file_time_type modifiedTime)
{
    for (CacheEntry& entry : entries_) {
        if (entry.key == key &&
            entry.renderer == renderer &&
            entry.fileSize == fileSize &&
            entry.modifiedTime == modifiedTime) {
            return &entry;
        }
    }
    return nullptr;
}

const ScenePlateImageCache::TextureResult& ScenePlateImageCache::textureFor(
    SDL_Renderer* renderer,
    const std::filesystem::path& imagePath)
{
    if (renderer == nullptr) {
        transientResult_ = makeTransientResult("renderer missing");
        return transientResult_;
    }

    if (imagePath.empty()) {
        transientResult_ = makeTransientResult("image path empty");
        return transientResult_;
    }

    std::error_code ec;
    if (!regularFileExistsForScenePlate(imagePath)) {
        transientResult_ = makeTransientResult("image file not found: " + imagePath.string());
        return transientResult_;
    }

    if (renderer_ != nullptr && renderer_ != renderer) {
        clear();
    }
    renderer_ = renderer;

    const std::string key = pathKey(imagePath);
    std::uintmax_t fileSize = std::filesystem::file_size(imagePath, ec);
    if (ec) {
        fileSize = 0;
        ec.clear();
    }
    std::filesystem::file_time_type modifiedTime = std::filesystem::last_write_time(imagePath, ec);
    if (ec) {
        modifiedTime = std::filesystem::file_time_type{};
        ec.clear();
    }

    if (CacheEntry* cached = findEntry(key, renderer, fileSize, modifiedTime)) {
        return cached->result;
    }

    LoadedRgbaImage image = loadImageWithWic(imagePath);

    CacheEntry entry;
    entry.renderer = renderer;
    entry.key = key;
    entry.fileSize = fileSize;
    entry.modifiedTime = modifiedTime;
    entry.result.attempted = true;
    entry.result.status = image.status;

    if (!image.ok || image.pixels.empty() || image.width <= 0 || image.height <= 0) {
        entry.result.ok = false;
        entries_.push_back(std::move(entry));
        return entries_.back().result;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC,
        image.width,
        image.height);
    if (texture == nullptr) {
        entry.result.ok = false;
        entry.result.status = std::string("SDL texture create failed: ") + SDL_GetError();
        entries_.push_back(std::move(entry));
        return entries_.back().result;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    if (!SDL_UpdateTexture(texture, nullptr, image.pixels.data(), image.width * 4)) {
        entry.result.ok = false;
        entry.result.status = std::string("SDL texture update failed: ") + SDL_GetError();
        SDL_DestroyTexture(texture);
        entries_.push_back(std::move(entry));
        return entries_.back().result;
    }

    entry.result.texture = texture;
    entry.result.width = image.width;
    entry.result.height = image.height;
    entry.result.ok = true;
    entry.result.status = image.status;
    entries_.push_back(std::move(entry));
    return entries_.back().result;
}

} // namespace perapera
