// src/app/main.cpp
//
// このファイルは「ぺらぺらアニメ作り機」のアプリ本体です。
//
// Phase 3Aでは、簡易ペン描画の土台を追加します。
// ここでは、DrawingCanvasPanelを画面に表示し、
// 左ドラッグで仮の作画キャンバス上に線を描けるようにします。
//
// 注意:
// Phase 2で使っていた CanvasPreview はここでは使いません。
// 今回は「簡易作画キャンバス」を表示するため、DrawingCanvasPanelを使います。

#include <iostream>
#include <string>

#ifdef _WIN32
    // Windowsのコンソールで日本語をUTF-8表示しやすくするために使う。
    #include <windows.h>
#endif

// SDL3は、ウィンドウ作成、入力処理、描画器作成に使う。
// このプロジェクトでは、OSごとの差をSDL3に吸収してもらう。
#include <SDL3/SDL.h>

// Dear ImGuiは、開発初期のメニューや設定パネルを作るために使う。
// 本格的なUIを作る前に、内部状態を確認できる道具として便利。
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

// WorkCanvasは、実際に絵を描く広い作画用キャンバスを表す。
#include "drawing/WorkCanvas.h"

// RenderFormatは、撮影で切り出す最終出力フレームを表す。
#include "project/RenderFormat.h"

// DrawingCanvasPanelは、左ドラッグで線を描ける簡易作画キャンバスUI。
#include "ui/DrawingCanvasPanel.h"

namespace
{
    // ウィンドウの初期幅。
    constexpr int InitialWindowWidth = 1280;

    // ウィンドウの初期高さ。
    constexpr int InitialWindowHeight = 720;

    // アプリの表示名。
    // CMakeや実行ファイル名はASCIIにしているが、ユーザーに見える名前は日本語にする。
    constexpr const char* AppDisplayName = "ぺらぺらアニメ作り機";

    // 今の開発段階を表示するための文字列。
    constexpr const char* CurrentPhaseName = "Phase 3E: Onion skin foundation";
    // SDLエラーを表示するための補助関数。
    // SDLの関数が失敗したとき、SDL_GetError()で理由を確認できる。
    void printSdlError(const std::string& message)
    {
        std::cerr << message << std::endl;
        std::cerr << "SDL error: " << SDL_GetError() << std::endl;
    }

    // Dear ImGuiに日本語フォントを読み込ませる関数。
    //
    // Dear ImGuiの標準フォントは軽量だが、日本語の文字を持っていない。
    // そのため、日本語を表示したい場合は、日本語フォントを明示的に読み込む必要がある。
    //
    // Windowsには通常、C:\Windows\Fonts に日本語フォントが入っている。
    // ここでは候補を順番に試し、最初に見つかったものを使う。
    bool loadJapaneseFontForImGui()
    {
        ImGuiIO& io = ImGui::GetIO();

        // 日本語表示に使うフォントサイズ。
        constexpr float JapaneseFontSize = 18.0f;

        // Windowsでよく存在する日本語フォント候補。
        const char* fontCandidates[] = {
            "C:\\Windows\\Fonts\\meiryo.ttc",
            "C:\\Windows\\Fonts\\YuGothM.ttc",
            "C:\\Windows\\Fonts\\msgothic.ttc"
        };

        for (const char* fontPath : fontCandidates)
        {
            // GetGlyphRangesJapanese() は、日本語表示に必要な文字範囲を指定する。
            // これを指定しないと、フォントを読んでも日本語が表示されないことがある。
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
        std::cerr << "C:\\Windows\\Fonts に meiryo.ttc などがあるか確認してください。" << std::endl;

        return false;
    }

    // キャンバス設定用のImGuiパネルを描く。
    //
    // WorkCanvas:
    // 実際に絵を描く広い紙。
    //
    // RenderFormat:
    // 撮影で切り出して最終出力する映像サイズ。
    void drawCanvasSettingsPanel(
        perapera::WorkCanvas& workCanvas,
        perapera::RenderFormat& renderFormat
    )
    {
        ImGui::Begin("キャンバス設定");

        ImGui::Text("作画キャンバス");
        ImGui::InputInt("キャンバス幅 px", &workCanvas.widthPx);
        ImGui::InputInt("キャンバス高さ px", &workCanvas.heightPx);

        // 入力された値が小さすぎたり大きすぎたりしないように補正する。
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

        // 出力形式の値も安全な範囲に補正する。
        renderFormat.clampToReasonableValues();

        ImGui::Text(
            "出力表示比率: %.3f",
            static_cast<double>(renderFormat.displayAspectRatio())
        );

        ImGui::Separator();

        // よく使いそうなプリセット。
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

    // 開発状況を表示するパネル。
    //
    // 今どこまでできていて、何がまだ未実装かを明示する。
    // これにより、別のAIや別PCで作業しても現状を把握しやすくする。
    void drawDevelopmentStatusPanel()
    {
        ImGui::Begin("ぺらぺらアニメ作り機 - 開発状況");

        ImGui::Text("現在の段階:");
        ImGui::BulletText("%s", CurrentPhaseName);

        ImGui::Separator();

        ImGui::Text("この段階でできること:");
    ImGui::BulletText("作画キャンバスサイズを変更する");
    ImGui::BulletText("撮影フレームサイズを変更する");
    ImGui::BulletText("簡易作画キャンバスに左ドラッグで線を描く");
    ImGui::BulletText("ペン半径と色を変更する");
    ImGui::BulletText("複数レイヤーを追加・削除する");
    ImGui::BulletText("レイヤー表示/非表示と不透明度を変更する");
    ImGui::BulletText("現在の撮影フレームをPNG保存する");
    ImGui::BulletText("複数フレームを追加・選択・複製・削除する");
    ImGui::BulletText("前後フレームをオニオンスキン表示する");

    ImGui::Separator();

    ImGui::Text("まだ未実装:");
    ImGui::BulletText("レイヤー名変更");
    ImGui::BulletText("消しゴム");

    ImGui::BulletText("カメラとレンズ");
    ImGui::BulletText("3D作画補助");

        ImGui::End();
    }
}

int main()
{
#ifdef _WIN32
    // Windowsコンソールの出力文字コードをUTF-8にする。
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "ぺらぺらアニメ作り機を起動します。" << std::endl;
    std::cout << CurrentPhaseName << std::endl;

    // SDLを初期化する。
    // 今回はウィンドウ表示だけなので SDL_INIT_VIDEO を指定する。
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printSdlError("SDLの初期化に失敗しました。");
        return 1;
    }

    // SDLでウィンドウを作成する。
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

    // SDL_Rendererは、SDLで2D描画をするための描画器。
    // Phase 3AではDear ImGuiのUIを描画するために使う。
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr)
    {
        printSdlError("SDL_Rendererの作成に失敗しました。");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Dear ImGuiのバージョン確認。
    IMGUI_CHECKVERSION();

    // Dear ImGuiの状態を保存するContextを作る。
    ImGui::CreateContext();

    // ImGuiIOには、入力、フォント、設定などの情報が入る。
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Dear ImGuiに日本語フォントを読み込ませる。
    loadJapaneseFontForImGui();

    // Dear ImGuiの標準的な暗色テーマを使う。
    ImGui::StyleColorsDark();

    // Dear ImGuiに「SDL3から入力イベントを受け取る」準備をさせる。
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);

    // Dear ImGuiに「SDL_Rendererで描画する」準備をさせる。
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool shouldQuit = false;
    bool showDemoWindow = false;

    // 作画キャンバス。
    // これは最終出力より広くできる「絵を描く紙」。
    perapera::WorkCanvas workCanvas;

    // 出力形式。
    // これは撮影で切り出す最終映像サイズ。
    perapera::RenderFormat renderFormat;

    // ここが重要。
    // Phase 2のCanvasPreviewではなく、
    // Phase 3AのDrawingCanvasPanelを画面に表示する。
    perapera::DrawingCanvasPanel drawingCanvasPanel;

    // メインループ。
    // アプリは終了命令が来るまでこのループを繰り返す。
    while (!shouldQuit)
    {
        SDL_Event event;

        // SDLに届いたイベントをすべて処理する。
        while (SDL_PollEvent(&event))
        {
            // Dear ImGuiにも入力イベントを渡す。
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

        // Dear ImGuiの新しいフレームを開始する。
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // メインメニューを作る。
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

        // 左側などに出る開発状況パネル。
        drawDevelopmentStatusPanel();

        // キャンバスサイズと出力フレームサイズを操作するパネル。
        drawCanvasSettingsPanel(workCanvas, renderFormat);

        // ここで簡易作画キャンバスを表示する。
        // このウィンドウ内で左ドラッグすると線が描ける。
        drawingCanvasPanel.draw(workCanvas, renderFormat);

        if (showDemoWindow)
        {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        // Dear ImGuiの描画コマンドを確定する。
        ImGui::Render();

        // 背景色を設定して画面をクリアする。
        SDL_SetRenderDrawColor(renderer, 20, 22, 26, 255);
        SDL_RenderClear(renderer);

        // Dear ImGuiをSDL_Rendererで描画する。
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        // 描画結果を画面に表示する。
        SDL_RenderPresent(renderer);
    }

    // 終了処理。
    // 作った順番と逆順に片付ける。
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "ぺらぺらアニメ作り機を終了しました。" << std::endl;

    return 0;
}