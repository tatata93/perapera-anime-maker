#pragma once
// このファイルの役割:
// Scene Plate の画像パスからキャンバス表示用SDL_Textureを遅延生成・キャッシュする。
// 画像読み込みはUI/ScenePlateモデルへ混ぜず、表示用キャッシュとしてAppが保持する。

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

namespace perapera {

class ScenePlateImageCache {
public:
    struct TextureResult {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
        bool ok = false;
        bool attempted = false;
        std::string status;
    };

    ScenePlateImageCache() = default;
    ScenePlateImageCache(const ScenePlateImageCache&) = delete;
    ScenePlateImageCache& operator=(const ScenePlateImageCache&) = delete;
    ~ScenePlateImageCache();

    void clear();

    const TextureResult& textureFor(SDL_Renderer* renderer, const std::filesystem::path& imagePath);

private:
    struct CacheEntry {
        TextureResult result;
        SDL_Renderer* renderer = nullptr;
        std::string key;
        std::uintmax_t fileSize = 0;
        std::filesystem::file_time_type modifiedTime{};
    };

    std::vector<CacheEntry> entries_;
    TextureResult transientResult_;
    SDL_Renderer* renderer_ = nullptr;

    CacheEntry* findEntry(const std::string& key,
                          SDL_Renderer* renderer,
                          std::uintmax_t fileSize,
                          std::filesystem::file_time_type modifiedTime);
};

} // namespace perapera
