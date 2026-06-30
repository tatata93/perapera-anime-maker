#include "ui/ScenePlateImageCache.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace {

namespace fs = std::filesystem;

int gFailureCount = 0;

void require(bool condition, std::string_view label)
{
    if (condition) {
        std::cout << "[OK] " << label << '\n';
        return;
    }
    ++gFailureCount;
    std::cerr << "[NG] " << label << '\n';
}

void writeSmallFile(const fs::path& path)
{
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << "not-a-real-image";
}

void runSelfTest()
{
    const fs::path root = fs::temp_directory_path() / "perapera_scene_plate_image_path_selftest";
    fs::remove_all(root);

    const fs::path projectFolder = root / "project";
    const fs::path imagePath = root / "Desktop" / "P1326073.JPG";
    writeSmallFile(imagePath);

    const perapera::ScenePlateImagePathInfo absoluteInfo =
        perapera::resolveScenePlateImagePathInfo(imagePath.string(), projectFolder);
    require(absoluteInfo.hasInput, "absolute path has input");
    require(absoluteInfo.isAbsolute, "absolute path is detected as absolute");
    require(absoluteInfo.inputExists, "absolute input exists");
    require(absoluteInfo.resolvedExists, "absolute resolved path exists");
    require(absoluteInfo.resolvedPath == imagePath, "absolute path is not joined with project folder");
    require(absoluteInfo.extensionSupported, "uppercase JPG extension is supported");

    const perapera::ScenePlateImagePathInfo quotedInfo =
        perapera::resolveScenePlateImagePathInfo("\"" + imagePath.string() + "\"\r\n", projectFolder);
    require(quotedInfo.normalizedText == imagePath.string(), "quotes and CR/LF are stripped");
    require(quotedInfo.isAbsolute, "quoted absolute path is still absolute");
    require(quotedInfo.resolvedExists, "quoted absolute path resolves to existing file");

    const fs::path relativeImagePath = projectFolder / "plates" / "layout.JPG";
    writeSmallFile(relativeImagePath);
    const perapera::ScenePlateImagePathInfo relativeInfo =
        perapera::resolveScenePlateImagePathInfo("plates/layout.JPG", projectFolder);
    require(!relativeInfo.isAbsolute, "relative path is detected as relative");
    require(relativeInfo.resolvedPath == relativeImagePath, "relative path is joined with project folder");
    require(relativeInfo.resolvedExists, "relative resolved path exists");

#if defined(_WIN32)
    std::string fullwidthYenPath = imagePath.string();
    std::string::size_type pos = 0;
    while ((pos = fullwidthYenPath.find('\\', pos)) != std::string::npos) {
        fullwidthYenPath.replace(pos, 1, reinterpret_cast<const char*>(u8"￥"));
        pos += std::string(reinterpret_cast<const char*>(u8"￥")).size();
    }
    const perapera::ScenePlateImagePathInfo yenInfo =
        perapera::resolveScenePlateImagePathInfo(fullwidthYenPath, projectFolder);
    require(yenInfo.isAbsolute, "fullwidth yen path is normalized as absolute");
    require(yenInfo.resolvedExists, "fullwidth yen path resolves to existing file");
#endif

    fs::remove_all(root);
}

} // namespace

int main()
{
    runSelfTest();
    if (gFailureCount == 0) {
        std::cout << "Scene plate image path self-test passed\n";
        return 0;
    }

    std::cerr << "Scene plate image path self-test failed: " << gFailureCount << " failure(s)\n";
    return 1;
}
