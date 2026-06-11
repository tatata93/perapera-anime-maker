// src/app/main.cpp
//
// このファイルは「ぺらぺらアニメ作り機」の最小アプリ本体です。
//
// Phase 1では、SDL3でウィンドウを作り、Dear ImGuiで簡単なUIを表示します。
// 今回の修正では、Dear ImGuiに日本語フォントを読み込ませます。
// これにより、ImGui内で「????」と表示されていた日本語を正しく表示します。

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

namespace
{
    // ウィンドウの初期幅。
    // 後で設定ファイルやプロジェクト設定から変更できるようにする予定。
    constexpr int InitialWindowWidth = 1280;

    // ウィンドウの初期高さ。
    constexpr int InitialWindowHeight = 720;

    // アプリの表示名。
    // CMakeや実行ファイル名はASCIIにしているが、ユーザーに見える名前は日本語にする。
    constexpr const char* AppDisplayName = "ぺらぺらアニメ作り機";

    // 今の開発段階を表示するための文字列。
    constexpr const char* CurrentPhaseName = "Phase 1: SDL3 + Dear ImGui minimal window";

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
        // 18pxくらいにすると、メニューやパネルの文字が読みやすい。
        constexpr float JapaneseFontSize = 18.0f;

        // Windowsでよく存在する日本語フォント候補。
        //
        // meiryo.ttc    : メイリオ
        // YuGothM.ttc   : 游ゴシック Medium
        // msgothic.ttc  : MS ゴシック
        //
        // .ttc は複数フォントを含む形式だが、Dear ImGuiの
        // AddFontFromFileTTF で読み込める場合が多い。
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

        // ここに来た場合、日本語フォントが見つからなかった。
        // アプリ自体は起動できるが、ImGui内の日本語は表示できない可能性が高い。
        std::cerr << "Dear ImGui用の日本語フォントを読み込めませんでした。" << std::endl;
        std::cerr << "C:\\Windows\\Fonts に meiryo.ttc などがあるか確認してください。" << std::endl;

        return false;
    }
}

int main()
{
#ifdef _WIN32
    // Windowsコンソールの出力文字コードをUTF-8にする。
    // これにより、日本語の「ぺらぺらアニメ作り機」が文字化けしにくくなる。
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
    // SDL_WINDOW_RESIZABLE により、ユーザーがウィンドウサイズを変えられる。
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
    // Phase 1では、Dear ImGuiのUIを描画するために使う。
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    if (renderer == nullptr)
    {
        printSdlError("SDL_Rendererの作成に失敗しました。");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Dear ImGuiのバージョン確認。
    // ライブラリ本体とヘッダーの不一致を検出するために使う。
    IMGUI_CHECKVERSION();

    // Dear ImGuiの状態を保存するContextを作る。
    ImGui::CreateContext();

    // ImGuiIOには、入力、フォント、設定などの情報が入る。
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Dear ImGuiに日本語フォントを読み込ませる。
    // これを行わないと、ImGuiのウィンドウ内で日本語が「????」になる。
    loadJapaneseFontForImGui();

    // Dear ImGuiの標準的な暗色テーマを使う。
    ImGui::StyleColorsDark();

    // Dear ImGuiに「SDL3から入力イベントを受け取る」準備をさせる。
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);

    // Dear ImGuiに「SDL_Rendererで描画する」準備をさせる。
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool shouldQuit = false;
    bool showDemoWindow = false;

    // メインループ。
    // アプリは、終了命令が来るまでこのループを繰り返す。
    while (!shouldQuit)
    {
        SDL_Event event;

        // SDLに届いたイベントをすべて処理する。
        // キーボード、マウス、ウィンドウ閉じるボタンなどがここに来る。
        while (SDL_PollEvent(&event))
        {
            // Dear ImGuiにも入力イベントを渡す。
            // これにより、ImGuiのボタンやメニューがマウス操作に反応する。
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

        // 最初の状態確認用ウィンドウ。
        ImGui::Begin("ぺらぺらアニメ作り機 - 開発状況");

        ImGui::Text("現在の段階:");
        ImGui::BulletText("%s", CurrentPhaseName);

        ImGui::Separator();

        ImGui::Text("この段階でできること:");
        ImGui::BulletText("SDL3でウィンドウを表示する");
        ImGui::BulletText("Dear ImGuiでメニューとパネルを表示する");
        ImGui::BulletText("ファイル > 終了 またはウィンドウの閉じるボタンで終了する");

        ImGui::Separator();

        ImGui::Text("まだ未実装:");
        ImGui::BulletText("作画キャンバス");
        ImGui::BulletText("カメラとレンズ");
        ImGui::BulletText("3D作画補助");
        ImGui::BulletText("背景画角キャリブレーション");
        ImGui::BulletText("簡易物理");

        ImGui::Separator();

        if (ImGui::Button("終了"))
        {
            shouldQuit = true;
        }

        ImGui::End();

        // Dear ImGuiのデモウィンドウ。
        // UI部品の例を見られるので、開発初期には便利。
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