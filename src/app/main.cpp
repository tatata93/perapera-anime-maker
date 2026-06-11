// src/app/main.cpp
//
// このファイルは「ぺらぺらアニメ作り機」のアプリ本体です。
//
// Phase 2では、任意サイズの作画キャンバスと撮影フレームの土台を追加します。
// まだ本格的なペン描画は実装しません。
// まず「作画キャンバス」と「最終出力フレーム」を別々に管理できるようにします。

#include <iostream>
#include <string>

#ifdef _WIN32
    #include <windows.h>
#endif

#include <SDL3/SDL.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"
#include "ui/CanvasPreview.h"

namespace
{
    constexpr int InitialWindowWidth = 1280;
    constexpr int InitialWindowHeight = 720;

    constexpr const char* AppDisplayName = "ぺらぺらアニメ作り機";
    constexpr const char* CurrentPhaseName = "Phase 2: Work canvas and render frame foundation";

    void printSdlError(const std::string& message)
    {
        std::cerr << message << std::endl;
        std::cerr << "SDL error: " << SDL_GetError() << std::endl;
    }

    bool loadJapaneseFontForImGui()
    {
        ImGuiIO& io = ImGui::GetIO();

        constexpr float JapaneseFontSize = 18.0f;

        const char* fontCandidates[] = {
            "C:\\Windows\\Fonts\\meiryo.ttc",
            "C:\\Windows\\Fonts\\YuGothM.ttc",
            "C:\\Windows\\Fonts\\msgothic.ttc"
        };

        for (const char* fontPath : fontCandidates)
        {
            ImFont* font = io.Fonts->AddFontFromFileTTF(
                fontPath,
                JapaneseFontSize,
                nullptr,
                io.Fonts->GetGlyphRangesJapanese()
            );

            if (font != nullptr)
            {
                std::cout << "Dear ImGui Japanese font loaded: " << fontPath << std::endl;
                return true;
            }
        }

        std::cerr << "Dear ImGui用の日本語フォントを読み込めませんでした。" << std::endl;
        return false;
    }

    // キャンバス設定用のImGuiパネルを描く。
    //
    // Phase 2では、このパネルで作画キャンバスの大きさと
    // 最終出力フレームの大きさを変更できるようにする。
    void drawCanvasSettingsPanel(
        perapera::WorkCanvas& workCanvas,
        perapera::RenderFormat& renderFormat
    )
    {
        ImGui::Begin("キャンバス設定");

        ImGui::Text("作画キャンバス");
        ImGui::InputInt("キャンバス幅 px", &workCanvas.widthPx);
        ImGui::InputInt("キャンバス高さ px", &workCanvas.heightPx);

        workCanvas.clampToReasonableSize();

        ImGui::Text(
            "作画キャンバス比率: %.3f",
            static_cast<double>(workCanvas.aspectRatio())
        );

        ImGui::Separator();

        ImGui::Text("撮影・出力フレーム");
        ImGui::InputInt("出力幅 px", &renderFormat.outputWidthPx);
        ImGui::InputInt("出力高さ px", &renderFormat.outputHeightPx);
        ImGui::InputInt("FPS", &renderFormat.framesPerSecond);
        ImGui::InputFloat("ピクセル縦横比", &renderFormat.pixelAspectRatio, 0.01f, 0.1f);

        renderFormat.clampToReasonableValues();

        ImGui::Text(
            "出力表示比率: %.3f",
            static_cast<double>(renderFormat.displayAspectRatio())
        );

        ImGui::Separator();

        if (ImGui::Button("標準 3840 x 2160 / 1920 x 1080"))
        {
            workCanvas.widthPx = 3840;
            workCanvas.heightPx = 2160;
            renderFormat.outputWidthPx = 1920;
            renderFormat.outputHeightPx = 1080;
            renderFormat.framesPerSecond = 24;
            renderFormat.pixelAspectRatio = 1.0f;
        }

        if (ImGui::Button("横パン用 6000 x 1080 / 1920 x 1080"))
        {
            workCanvas.widthPx = 6000;
            workCanvas.heightPx = 1080;
            renderFormat.outputWidthPx = 1920;
            renderFormat.outputHeightPx = 1080;
            renderFormat.framesPerSecond = 24;
            renderFormat.pixelAspectRatio = 1.0f;
        }

        if (ImGui::Button("縦動画用 2160 x 3840 / 1080 x 1920"))
        {
            workCanvas.widthPx = 2160;
            workCanvas.heightPx = 3840;
            renderFormat.outputWidthPx = 1080;
            renderFormat.outputHeightPx = 1920;
            renderFormat.framesPerSecond = 24;
            renderFormat.pixelAspectRatio = 1.0f;
        }

        ImGui::End();
    }

    void drawDevelopmentStatusPanel()
    {
        ImGui::Begin("ぺらぺらアニメ作り機 - 開発状況");

        ImGui::Text("現在の段階:");
        ImGui::BulletText("%s", CurrentPhaseName);

        ImGui::Separator();

        ImGui::Text("この段階でできること:");
        ImGui::BulletText("作画キャンバスサイズを変更する");
        ImGui::BulletText("撮影フレームサイズを変更する");
        ImGui::BulletText("作画キャンバスと撮影フレームを仮表示する");
        ImGui::BulletText("出力フレームがキャンバスからはみ出す場合に警告する");

        ImGui::Separator();

        ImGui::Text("まだ未実装:");
        ImGui::BulletText("ペン描画");
        ImGui::BulletText("レイヤー");
        ImGui::BulletText("カメラとレンズ");
        ImGui::BulletText("3D作画補助");
        ImGui::BulletText("背景画角キャリブレーション");
        ImGui::BulletText("簡易物理");

        ImGui::End();
    }
}

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "ぺらぺらアニメ作り機を起動します。" << std::endl;
    std::cout << CurrentPhaseName << std::endl;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printSdlError("SDLの初期化に失敗しました。");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        AppDisplayName,
        InitialWindowWidth,
        InitialWindowHeight,
        SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr)
    {
        printSdlError("ウィンドウ作成に失敗しました。");
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr)
    {
        printSdlError("SDL_Rendererの作成に失敗しました。");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    loadJapaneseFontForImGui();

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool shouldQuit = false;
    bool showDemoWindow = false;

    perapera::WorkCanvas workCanvas;
    perapera::RenderFormat renderFormat;
    perapera::CanvasPreview canvasPreview;

    while (!shouldQuit)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                shouldQuit = true;
            }

            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                shouldQuit = true;
            }
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("ファイル"))
            {
                if (ImGui::MenuItem("終了"))
                {
                    shouldQuit = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("表示"))
            {
                ImGui::MenuItem("Dear ImGui デモ", nullptr, &showDemoWindow);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("ヘルプ"))
            {
                ImGui::MenuItem("このソフトについて", nullptr, false, false);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        drawDevelopmentStatusPanel();
        drawCanvasSettingsPanel(workCanvas, renderFormat);
        canvasPreview.draw(workCanvas, renderFormat);

        if (showDemoWindow)
        {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 20, 22, 26, 255);
        SDL_RenderClear(renderer);

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "ぺらぺらアニメ作り機を終了しました。" << std::endl;

    return 0;
}