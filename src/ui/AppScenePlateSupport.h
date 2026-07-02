// Scene Plate UI/canvas helpers split from AppDrawingMode.
#pragma once

#include "ui/App.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>

#include <imgui.h>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace perapera::scene_plate_ui {

inline const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

inline int scenePlateKindIndex(ScenePlateKind kind) noexcept
{
    switch (kind) {
    case ScenePlateKind::Storyboard:
        return 0;
    case ScenePlateKind::Layout:
        return 1;
    case ScenePlateKind::ReferenceImage:
        return 2;
    case ScenePlateKind::TemporaryBackground:
        return 3;
    case ScenePlateKind::FinalBackground:
        return 4;
    }
    return 0;
}

inline ScenePlateKind scenePlateKindFromIndex(int index) noexcept
{
    switch (std::clamp(index, 0, 4)) {
    case 1:
        return ScenePlateKind::Layout;
    case 2:
        return ScenePlateKind::ReferenceImage;
    case 3:
        return ScenePlateKind::TemporaryBackground;
    case 4:
        return ScenePlateKind::FinalBackground;
    case 0:
    default:
        return ScenePlateKind::Storyboard;
    }
}

inline int scenePlateOutputModeIndex(ScenePlateOutputMode mode) noexcept
{
    switch (mode) {
    case ScenePlateOutputMode::ReferenceOnly:
        return 0;
    case ScenePlateOutputMode::PreviewOnly:
        return 1;
    case ScenePlateOutputMode::RenderOutput:
        return 2;
    }
    return 0;
}

inline ScenePlateOutputMode scenePlateOutputModeFromIndex(int index) noexcept
{
    switch (std::clamp(index, 0, 2)) {
    case 1:
        return ScenePlateOutputMode::PreviewOnly;
    case 2:
        return ScenePlateOutputMode::RenderOutput;
    case 0:
    default:
        return ScenePlateOutputMode::ReferenceOnly;
    }
}

inline std::string defaultScenePlateName(ScenePlateKind kind, int number)
{
    return std::string(scenePlateKindJapaneseLabel(kind)) + " " + std::to_string(std::max(1, number));
}

inline std::string nextScenePlateId(const ScenePlateStack& stack)
{
    int number = static_cast<int>(stack.plates.size()) + 1;
    for (;;) {
        const std::string candidate = "plate_" + std::to_string(number);
        const auto found = std::find_if(stack.plates.begin(), stack.plates.end(), [&](const ScenePlate& plate) {
            return plate.id == candidate;
        });
        if (found == stack.plates.end()) {
            return candidate;
        }
        ++number;
    }
}

inline int nextScenePlateZOrder(const ScenePlateStack& stack) noexcept
{
    int zOrder = 0;
    for (const ScenePlate& plate : stack.plates) {
        zOrder = std::max(zOrder, plate.zOrder + 1);
    }
    return zOrder;
}

inline ScenePlate makeDefaultScenePlate(const ScenePlateStack& stack, ScenePlateKind kind)
{
    ScenePlate plate;
    plate.id = nextScenePlateId(stack);
    plate.kind = kind;
    plate.outputMode = ScenePlateOutputMode::ReferenceOnly;
    if (kind == ScenePlateKind::TemporaryBackground) {
        plate.outputMode = ScenePlateOutputMode::PreviewOnly;
    } else if (kind == ScenePlateKind::FinalBackground) {
        plate.outputMode = ScenePlateOutputMode::RenderOutput;
    }
    plate.displayName = defaultScenePlateName(kind, static_cast<int>(stack.plates.size()) + 1);
    plate.opacity = 0.5f;
    plate.visible = true;
    plate.locked = false;
    plate.zOrder = nextScenePlateZOrder(stack);
    normalizeScenePlate(plate);
    return plate;
}

inline void clampScenePlateSelection(const ScenePlateStack& stack, int& selectedIndex) noexcept
{
    if (stack.plates.empty()) {
        selectedIndex = 0;
        return;
    }
    selectedIndex = std::clamp(selectedIndex, 0, static_cast<int>(stack.plates.size()) - 1);
}

inline void assignScenePlateSequentialZOrder(ScenePlateStack& stack) noexcept
{
    for (int index = 0; index < static_cast<int>(stack.plates.size()); ++index) {
        stack.plates[static_cast<std::size_t>(index)].zOrder = index;
    }
}

inline bool inputScenePlateText(const char* label, std::string& value)
{
    std::array<char, 1024> buffer{};
    const std::size_t copyLength = std::min(value.size(), buffer.size() - 1U);
    std::copy_n(value.c_str(), copyLength, buffer.data());
    if (ImGui::InputText(label, buffer.data(), buffer.size())) {
        value = buffer.data();
        return true;
    }
    return false;
}


inline void scenePlateReplaceAll(std::string& text, const std::string& from, const std::string& to)
{
    if (from.empty()) {
        return;
    }
    std::size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

inline std::string trimScenePlateImagePathText(std::string text)
{
    return normalizeScenePlateImagePathText(std::move(text));
}

#if defined(_WIN32)
inline std::wstring scenePlateWidePathFromUtf8OrAnsi(const std::string& cleanPathText)
{
    if (cleanPathText.empty()) {
        return {};
    }

    auto convert = [&](UINT codePage, DWORD flags) -> std::wstring {
        const int wideLength = MultiByteToWideChar(
            codePage,
            flags,
            cleanPathText.c_str(),
            static_cast<int>(cleanPathText.size()),
            nullptr,
            0);
        if (wideLength <= 0) {
            return {};
        }

        std::wstring widePath(static_cast<std::size_t>(wideLength), L'\0');
        const int convertedLength = MultiByteToWideChar(
            codePage,
            flags,
            cleanPathText.c_str(),
            static_cast<int>(cleanPathText.size()),
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

inline bool scenePlateWin32RegularFileExistsW(const std::wstring& widePath)
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

inline bool scenePlateWin32RegularFileExistsA(const std::string& pathText)
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
#endif

inline std::filesystem::path scenePlatePathFromUtf8Text(const std::string& cleanPathText)
{
    return scenePlatePathFromImageText(cleanPathText);
}

inline bool scenePlateRegularFileExists(const std::filesystem::path& path)
{
    return scenePlateImagePathExists(path);
}

inline std::string scenePlateImageExtensionLower(const std::filesystem::path& path)
{
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension;
}

inline bool scenePlateImageExtensionSupported(const std::filesystem::path& path)
{
    const std::string extension = scenePlateImageExtensionLower(path);
    return extension == ".png" ||
        extension == ".jpg" ||
        extension == ".jpeg" ||
        extension == ".bmp" ||
        extension == ".webp";
}

inline std::filesystem::path scenePlateResolvedImagePath(const ScenePlate& plate, const std::filesystem::path& projectFolder)
{
    return resolveScenePlateImagePathInfo(plate.imagePath, projectFolder).resolvedPath;
}

inline std::string scenePlateImageFileNameLabel(const ScenePlate& plate)
{
    const std::string cleanPathText = trimScenePlateImagePathText(plate.imagePath);
    if (cleanPathText.empty()) {
        return u8c(u8"未指定");
    }
    const std::filesystem::path path = scenePlatePathFromUtf8Text(cleanPathText);
    const std::string filename = path.filename().string();
    return filename.empty() ? cleanPathText : filename;
}

inline std::string scenePlateImageStatusLabel(const ScenePlate& plate, const std::filesystem::path& projectFolder)
{
    if (trimScenePlateImagePathText(plate.imagePath).empty()) {
        return u8c(u8"画像未指定: ダミー表示");
    }

    const std::filesystem::path resolvedPath = scenePlateResolvedImagePath(plate, projectFolder);
    const ScenePlateImagePathInfo pathInfo = resolveScenePlateImagePathInfo(plate.imagePath, projectFolder);
    if (!pathInfo.resolvedExists) {
        return std::string(u8c(u8"画像未検出: ")) + scenePlateImageFileNameLabel(plate);
    }

    if (!scenePlateImageExtensionSupported(resolvedPath)) {
        const std::string extension = scenePlateImageExtensionLower(resolvedPath);
        return std::string(u8c(u8"未対応拡張子: ")) + (extension.empty() ? std::string(u8c(u8"なし")) : extension);
    }

    return std::string(u8c(u8"画像パスOK: ")) + scenePlateImageFileNameLabel(plate);
}

inline std::string scenePlateImageDetailLabel(const ScenePlate& plate, const std::filesystem::path& projectFolder)
{
    if (trimScenePlateImagePathText(plate.imagePath).empty()) {
        return u8c(u8"画像パスを指定すると、キャンバス仮表示で実画像表示を試します。");
    }

    const std::filesystem::path resolvedPath = scenePlateResolvedImagePath(plate, projectFolder);
    const ScenePlateImagePathInfo pathInfo = resolveScenePlateImagePathInfo(plate.imagePath, projectFolder);
    if (!pathInfo.resolvedExists) {
        return std::string(u8c(u8"解決パス: ")) + resolvedPath.string();
    }

    std::error_code ec;
    const auto fileSize = std::filesystem::file_size(resolvedPath, ec);
    std::ostringstream stream;
    stream << u8c(u8"解決パス: ") << resolvedPath.string();
    if (!ec) {
        stream << u8c(u8" / ") << fileSize << u8c(u8" bytes");
    }
    return stream.str();
}

inline bool scenePlateMakeImagePathRelativeToProject(ScenePlate& plate, const std::filesystem::path& projectFolder)
{
    const std::string cleanPathText = trimScenePlateImagePathText(plate.imagePath);
    if (cleanPathText.empty() || projectFolder.empty()) {
        return false;
    }

    const std::filesystem::path imagePath = scenePlatePathFromUtf8Text(cleanPathText);
    if (!imagePath.is_absolute()) {
        if (plate.imagePath != cleanPathText) {
            plate.imagePath = cleanPathText;
            return true;
        }
        return false;
    }

    std::error_code ec;
    const std::filesystem::path relativePath = std::filesystem::relative(imagePath, projectFolder, ec);
    if (ec || relativePath.empty()) {
        if (plate.imagePath != cleanPathText) {
            plate.imagePath = cleanPathText;
            return true;
        }
        return false;
    }

    plate.imagePath = relativePath.generic_string();
    return true;
}

inline int scenePlateAlphaByte(float opacity, float minimumAlpha, float maximumAlpha) noexcept
{
    const float alpha = std::clamp(minimumAlpha + std::clamp(opacity, 0.0f, 1.0f) * (maximumAlpha - minimumAlpha), 0.0f, 1.0f);
    return std::clamp(static_cast<int>(alpha * 255.0f + 0.5f), 0, 255);
}

inline ImU32 scenePlateKindColor(ScenePlateKind kind, int alpha) noexcept
{
    switch (kind) {
    case ScenePlateKind::Storyboard:
        return IM_COL32(255, 225, 90, alpha);
    case ScenePlateKind::Layout:
        return IM_COL32(80, 170, 255, alpha);
    case ScenePlateKind::ReferenceImage:
        return IM_COL32(180, 140, 255, alpha);
    case ScenePlateKind::TemporaryBackground:
        return IM_COL32(80, 210, 170, alpha);
    case ScenePlateKind::FinalBackground:
        return IM_COL32(255, 150, 95, alpha);
    }
    return IM_COL32(200, 200, 200, alpha);
}

inline ImU32 scenePlateOutputBorderColor(ScenePlateOutputMode mode, int alpha) noexcept
{
    switch (mode) {
    case ScenePlateOutputMode::ReferenceOnly:
        return IM_COL32(80, 80, 80, alpha);
    case ScenePlateOutputMode::PreviewOnly:
        return IM_COL32(40, 115, 210, alpha);
    case ScenePlateOutputMode::RenderOutput:
        return IM_COL32(210, 80, 35, alpha);
    }
    return IM_COL32(80, 80, 80, alpha);
}

inline ImVec2 scenePlateRotatedPoint(float x, float y, float centerX, float centerY, float cosTheta, float sinTheta) noexcept
{
    const float dx = x - centerX;
    const float dy = y - centerY;
    return ImVec2(
        centerX + dx * cosTheta - dy * sinTheta,
        centerY + dx * sinTheta + dy * cosTheta);
}

inline void drawScenePlatePreviewOnCanvas(const ScenePlate& plate,
                                   int timelineFrame,
                                   int canvasWidth,
                                   int canvasHeight,
                                   const std::filesystem::path& projectFolder,
                                   SDL_Renderer* renderer,
                                   ScenePlateImageCache& imageCache,
                                   const CanvasView& view,
                                   ImVec2 areaMin,
                                   ImVec2 areaSize,
                                   ImDrawList* drawList)
{
    if (drawList == nullptr || canvasWidth <= 0 || canvasHeight <= 0) {
        return;
    }
    if (!scenePlateVisibleAtTimelineFrame(plate, timelineFrame)) {
        return;
    }

    const float opacity = std::clamp(plate.opacity, 0.0f, 1.0f);
    if (opacity <= 0.0f) {
        return;
    }

    const float width = std::max(16.0f, static_cast<float>(canvasWidth) * std::abs(plate.transform.scaleX));
    const float height = std::max(16.0f, static_cast<float>(canvasHeight) * std::abs(plate.transform.scaleY));
    const float left = plate.transform.x;
    const float top = plate.transform.y;
    const float right = left + width;
    const float bottom = top + height;
    const float centerX = (left + right) * 0.5f;
    const float centerY = (top + bottom) * 0.5f;
    constexpr float kPi = 3.14159265358979323846f;
    const float radians = plate.transform.rotationDegrees * kPi / 180.0f;
    const float cosTheta = std::cos(radians);
    const float sinTheta = std::sin(radians);

    const std::array<ImVec2, 4> canvasCorners{
        scenePlateRotatedPoint(left, top, centerX, centerY, cosTheta, sinTheta),
        scenePlateRotatedPoint(right, top, centerX, centerY, cosTheta, sinTheta),
        scenePlateRotatedPoint(right, bottom, centerX, centerY, cosTheta, sinTheta),
        scenePlateRotatedPoint(left, bottom, centerX, centerY, cosTheta, sinTheta),
    };

    std::array<ImVec2, 4> screenCorners{};
    for (std::size_t i = 0; i < canvasCorners.size(); ++i) {
        screenCorners[i] = view.canvasToScreen(canvasCorners[i].x, canvasCorners[i].y, areaMin, areaSize);
    }

    const ImVec2 screenCenter = view.canvasToScreen(centerX, centerY, areaMin, areaSize);
    const ImU32 borderColor = scenePlateOutputBorderColor(plate.outputMode, scenePlateAlphaByte(opacity, 0.42f, 0.95f));
    const float borderThickness = plate.outputMode == ScenePlateOutputMode::RenderOutput ? 3.0f : 2.0f;

    std::string imageState = scenePlateImageStatusLabel(plate, projectFolder);
    bool drewActualImage = false;
    const std::filesystem::path resolvedImagePath = scenePlateResolvedImagePath(plate, projectFolder);
    if (!plate.imagePath.empty() && scenePlateImageExtensionSupported(resolvedImagePath)) {
        const ScenePlateImageCache::TextureResult& textureResult = imageCache.textureFor(renderer, resolvedImagePath);
        imageState = textureResult.status.empty() ? imageState : textureResult.status;
        if (textureResult.ok && textureResult.texture != nullptr) {
            const int alpha = scenePlateAlphaByte(opacity, 0.0f, 1.0f);
            const ImU32 tintColor = IM_COL32(255, 255, 255, alpha);
            const ImTextureID textureId = reinterpret_cast<ImTextureID>(textureResult.texture);
            if (std::abs(plate.transform.rotationDegrees) <= 0.001f) {
                drawList->AddImage(
                    textureId,
                    screenCorners[0],
                    screenCorners[2],
                    ImVec2(0.0f, 0.0f),
                    ImVec2(1.0f, 1.0f),
                    tintColor);
            } else {
                drawList->AddImageQuad(
                    textureId,
                    screenCorners[0],
                    screenCorners[1],
                    screenCorners[2],
                    screenCorners[3],
                    ImVec2(0.0f, 0.0f),
                    ImVec2(1.0f, 0.0f),
                    ImVec2(1.0f, 1.0f),
                    ImVec2(0.0f, 1.0f),
                    tintColor);
            }
            drewActualImage = true;
        }
    }

    if (!drewActualImage) {
        const ImU32 fillColor = scenePlateKindColor(plate.kind, scenePlateAlphaByte(opacity, 0.05f, 0.24f));
        drawList->AddQuadFilled(screenCorners[0], screenCorners[1], screenCorners[2], screenCorners[3], fillColor);
    }

    drawList->AddQuad(screenCorners[0], screenCorners[1], screenCorners[2], screenCorners[3], borderColor, borderThickness);

    const std::string title = plate.displayName.empty() ? plate.id : plate.displayName;
    const std::string label = title + "\n" +
        scenePlateKindJapaneseLabel(plate.kind) + std::string(" / ") +
        scenePlateOutputModeJapaneseLabel(plate.outputMode) + "\n" +
        imageState;
    const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    const ImVec2 textMin(screenCenter.x - textSize.x * 0.5f - 6.0f,
                         screenCenter.y - textSize.y * 0.5f - 4.0f);
    const ImVec2 textMax(textMin.x + textSize.x + 12.0f,
                         textMin.y + textSize.y + 8.0f);

    const int labelBackgroundAlpha = drewActualImage
        ? scenePlateAlphaByte(opacity, 0.36f, 0.72f)
        : scenePlateAlphaByte(opacity, 0.58f, 0.86f);
    drawList->AddRectFilled(textMin, textMax, IM_COL32(255, 255, 255, labelBackgroundAlpha), 4.0f);
    drawList->AddRect(textMin, textMax, borderColor, 4.0f);
    drawList->AddText(
        ImVec2(textMin.x + 6.0f, textMin.y + 4.0f),
        IM_COL32(25, 25, 25, scenePlateAlphaByte(opacity, 0.72f, 1.0f)),
        label.c_str());
}

inline void drawScenePlatePreviewStackOnCanvas(const ScenePlateStack& stack,
                                        int timelineFrame,
                                        int canvasWidth,
                                        int canvasHeight,
                                        const std::filesystem::path& projectFolder,
                                        SDL_Renderer* renderer,
                                        ScenePlateImageCache& imageCache,
                                        const CanvasView& view,
                                        ImVec2 areaMin,
                                        ImVec2 areaSize,
                                        ImDrawList* drawList)
{
    if (drawList == nullptr || stack.plates.empty()) {
        return;
    }

    ScenePlateStack sortedStack = stack;
    normalizeScenePlateStack(sortedStack);

    const ImVec2 areaMax(areaMin.x + areaSize.x, areaMin.y + areaSize.y);
    drawList->PushClipRect(areaMin, areaMax, true);
    for (const ScenePlate& plate : sortedStack.plates) {
        drawScenePlatePreviewOnCanvas(
            plate,
            timelineFrame,
            canvasWidth,
            canvasHeight,
            projectFolder,
            renderer,
            imageCache,
            view,
            areaMin,
            areaSize,
            drawList);
    }
    drawList->PopClipRect();
}

} // namespace perapera::scene_plate_ui
