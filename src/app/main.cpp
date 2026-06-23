// このファイルの役割:
// SDL3 / Dear ImGui の初期化と終了処理だけを行う薄いエントリーポイント。
// アプリ状態、作画UI、入力処理は src/ui/App.* に分離する。

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "ui/App.h"
#include "ui/Theme.h"

namespace {

constexpr int kInitialWindowWidth = 1280;
constexpr int kInitialWindowHeight = 800;

const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

ImVector<ImWchar> gJapaneseGlyphRanges;

std::vector<std::string> buildJapaneseFontCandidatePaths()
{
    return {
        "C:/Windows/Fonts/msgothic.ttc",
        "C:/Windows/Fonts/meiryo.ttc",
        "C:/Windows/Fonts/YuGothM.ttc",
        "C:/Windows/Fonts/YuGothR.ttc",
        "C:/Windows/Fonts/BIZ-UDGothicR.ttc",
    };
}

void addJapaneseUiText(ImFontGlyphRangesBuilder& builder)
{
    // Phase 0 + Phase 1 Step 1-4で実際に表示する日本語を登録する。
    // CJK全域を登録するとSDL_Rendererバックエンドでフォントテクスチャが大きくなりすぎるため、
    // 現時点で使う文字だけを最小限追加する。
    builder.AddText(u8c(u8"1 プロジェクト 2 絵コンテ 2.5 プリビズ 3 作画 4 撮影 5 出力"));
    builder.AddText(u8c(u8"ファイル 編集 表示 ウィンドウ ヘルプ 新規 開く 保存 別名保存 最近使ったファイル"));
    builder.AddText(u8c(u8"Undo Redo ズームイン ズームアウト 全体表示 グリッド表示 レイアウトリセット"));
    builder.AddText(u8c(u8"ぺらぺらアニメ作り機 左サイドバー 右サイドバー メインキャンバス タイムライン"));
    builder.AddText(u8c(u8"ブラシ 消しゴム サイズ 不透明度 色 手ぶれ補正 ツール カラー オニオンスキン"));
    builder.AddText(u8c(u8"レイヤー 一覧 追加 削除 クリア 表示 名前 濃度 通常 選択中 線画"));
    builder.AddText(u8c(u8"フレーム コマ 追加 複製 削除 尺 現在 前 次 再生 停止 プレイヘッド"));
    builder.AddText(u8c(u8"プロジェクト保存 読み込み フォルダ パス 書き出し PNG連番 MP4 FFmpeg 実行 ログ"));
    builder.AddText(u8c(u8"状態 保存しました 読み込みました 失敗 成功 出力先 作品タイトル キャンバス"));
    builder.AddText(u8c(u8"日本語テスト：作画 撮影 絵コンテ タイムライン"));
    builder.AddText(u8c(u8"線を引ける 消せる レイヤーの追加削除切り替え オニオンスキン Undo Redo"));
    builder.AddText(u8c(u8"まだ空です Phase 本実装 最小実装 接続済み"));
}

void setupJapaneseFont(ImGuiIO& io)
{
    io.Fonts->Clear();

    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    addJapaneseUiText(builder);
    builder.BuildRanges(&gJapaneseGlyphRanges);

    constexpr float kJapaneseFontSizePx = 18.0f;
    for (const std::string& fontPath : buildJapaneseFontCandidatePaths()) {
        std::error_code errorCode;
        if (!std::filesystem::exists(fontPath, errorCode)) {
            continue;
        }

        for (int fontNo = 0; fontNo < 4; ++fontNo) {
            ImFontConfig config;
            config.FontNo = fontNo;
            config.OversampleH = 1;
            config.OversampleV = 1;
            config.PixelSnapH = true;

            ImFont* font = io.Fonts->AddFontFromFileTTF(
                fontPath.c_str(),
                kJapaneseFontSizePx,
                &config,
                gJapaneseGlyphRanges.Data
            );

            if (font == nullptr) {
                continue;
            }

            io.FontDefault = font;
            std::fprintf(stderr, "Japanese UI font loaded: %s fontNo=%d\n", fontPath.c_str(), fontNo);
            return;
        }
    }

    io.Fonts->AddFontDefault();
    io.FontDefault = nullptr;
    std::fprintf(stderr, "Japanese UI font was not loaded. Fallback font used.\n");
}

} // namespace

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        u8c(u8"ぺらぺらアニメ作り機 - Phase 1"),
        kInitialWindowWidth,
        kInitialWindowHeight,
        SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    setupJapaneseFont(io);
    perapera::ui::applyDarkTheme();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    perapera::App app;
    app.setRenderer(renderer);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        app.draw();

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 13, 13, 16, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    app.setRenderer(nullptr);

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
