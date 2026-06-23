// このファイルの役割:
// アプリのエントリーポイント。
// Phase 0 では SDL3 のウィンドウ作成、Dear ImGui 初期化、
// モードタブの切り替え骨格だけを持つ。
// 作画データやレンダリングコアは Phase 1 以降で別ファイルに分離する。

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <array>
#include <cstdio>
#include <string_view>

#include "ui/Theme.h"

namespace {

constexpr int kInitialWindowWidth = 1280;
constexpr int kInitialWindowHeight = 800;

struct ModeInfo {
    const char* tabLabel;
    const char* title;
    const char* description;
};

enum class AppMode : int {
    Project = 0,
    Storyboard,
    Previs,
    Drawing,
    Export,
};

constexpr std::array<ModeInfo, 5> kModes = {{
    {"① プロジェクト", "プロジェクトモード", "作品とカットを管理する場所。Phase 0 では空パネルのみ。"},
    {"② 絵コンテ", "絵コンテモード", "絵コンテを描き、台詞・アクション・カメラ指示を書く場所。Phase 0 では空パネルのみ。"},
    {"②.5 プリビズ", "プリビズモード", "3D下敷きや作画補助を扱う場所。Phase 2.5 で本実装する。"},
    {"③ 作画", "作画モード", "ブラシ、レイヤー、フレーム、タイムラインを使って作画する場所。Phase 1 で本実装する。"},
    {"⑤ 出力", "出力モード", "PNG連番やMP4を書き出す場所。Phase 1 で最低限の書き出しを追加する。"},
}};

int modeToIndex(AppMode mode)
{
    return static_cast<int>(mode);
}

AppMode indexToMode(int index)
{
    if (index < 0 || index >= static_cast<int>(kModes.size())) {
        return AppMode::Project;
    }
    return static_cast<AppMode>(index);
}

void drawMainMenuBar(bool& showImguiDemo)
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("ファイル")) {
        ImGui::MenuItem("新規", nullptr, false, false);
        ImGui::MenuItem("開く", nullptr, false, false);
        ImGui::MenuItem("保存", "Ctrl+S", false, false);
        ImGui::MenuItem("別名保存", nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("編集")) {
        ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
        ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("表示")) {
        ImGui::MenuItem("ズームイン", nullptr, false, false);
        ImGui::MenuItem("ズームアウト", nullptr, false, false);
        ImGui::MenuItem("全体表示", "F", false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("ウィンドウ")) {
        ImGui::MenuItem("ImGuiデモ", nullptr, &showImguiDemo);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("ヘルプ")) {
        ImGui::TextUnformatted("ぺらぺらアニメ作り機 Phase 0");
        ImGui::TextDisabled("ショートカット一覧は後続フェーズで追加予定");
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void drawModeTabs(AppMode& currentMode)
{
    ImGui::BeginChild("ModeTabs", ImVec2(0.0f, 42.0f), true);

    for (int index = 0; index < static_cast<int>(kModes.size()); ++index) {
        if (index > 0) {
            ImGui::SameLine();
        }

        const bool selected = index == modeToIndex(currentMode);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, perapera::ui::themeColors().accent);
        }

        if (ImGui::Button(kModes[index].tabLabel, ImVec2(150.0f, 28.0f))) {
            currentMode = indexToMode(index);
        }

        if (selected) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 170.0f);
    if (ImGui::Button("レイアウトリセット▼", ImVec2(155.0f, 28.0f))) {
        ImGui::OpenPopup("LayoutResetPopup");
    }

    if (ImGui::BeginPopup("LayoutResetPopup")) {
        ImGui::MenuItem("このモードをリセット", nullptr, false, false);
        ImGui::MenuItem("全モードリセット", nullptr, false, false);
        ImGui::MenuItem("別モードのレイアウトを読み込む", nullptr, false, false);
        ImGui::EndPopup();
    }

    ImGui::EndChild();
}

void drawPlaceholderPanel(const char* title, const char* body)
{
    ImGui::BeginChild(title, ImVec2(0.0f, 0.0f), true);
    ImGui::TextUnformatted(title);
    ImGui::Separator();
    ImGui::TextWrapped("%s", body);
    ImGui::EndChild();
}

void drawModeWorkspace(AppMode currentMode)
{
    const ModeInfo& mode = kModes[modeToIndex(currentMode)];

    ImGui::BeginChild("Workspace", ImVec2(0.0f, -28.0f), true);
    ImGui::TextUnformatted(mode.title);
    ImGui::TextDisabled("%s", mode.description);
    ImGui::Separator();

    const float sideWidth = 240.0f;
    const float rightWidth = 280.0f;
    const float timelineHeight = 110.0f;
    const float availableHeight = ImGui::GetContentRegionAvail().y;
    const float centerHeight = availableHeight - timelineHeight - ImGui::GetStyle().ItemSpacing.y;

    ImGui::BeginChild("UpperArea", ImVec2(0.0f, centerHeight), false);

    ImGui::BeginChild("LeftSidebar", ImVec2(sideWidth, 0.0f), true);
    drawPlaceholderPanel("左サイドバー", "Phase 0では枠だけ。Phase 1でツール、ブラシ設定、カラー、オニオンスキンを入れる。");
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("CenterCanvas", ImVec2(-(rightWidth + ImGui::GetStyle().ItemSpacing.x), 0.0f), true);
    const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(
        canvasMin,
        ImVec2(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y),
        ImGui::ColorConvertFloat4ToU32(perapera::ui::themeColors().canvasBackground)
    );
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 16.0f);
    ImGui::TextUnformatted("メインキャンバス");
    ImGui::TextDisabled("ここに Phase 1 で CanvasRenderer を接続する");
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightSidebar", ImVec2(rightWidth, 0.0f), true);
    drawPlaceholderPanel("右サイドバー", "Phase 1でレイヤー、フレーム情報、書き出しパネルを入れる。Phase 2でセル一覧を追加する。");
    ImGui::EndChild();

    ImGui::EndChild();

    drawPlaceholderPanel("タイムライン", "Phase 0では枠だけ。Phase 1でフレーム追加、再生コントロール、プレイヘッドを入れる。");

    ImGui::EndChild();
}

void drawStatusBar()
{
    ImGui::BeginChild("StatusBar", ImVec2(0.0f, 24.0f), true);
    ImGui::TextUnformatted("fps: -- | canvas: -- | zoom: 100% | brush: -- px");
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 240.0f);
    ImGui::TextDisabled("memory: -- | GPU VRAM: --");
    ImGui::EndChild();
}

void drawAppUi(AppMode& currentMode, bool& showImguiDemo)
{
    drawMainMenuBar(showImguiDemo);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    const ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("MainLayout", nullptr, windowFlags);
    drawModeTabs(currentMode);
    drawModeWorkspace(currentMode);
    drawStatusBar();
    ImGui::End();

    if (showImguiDemo) {
        ImGui::ShowDemoWindow(&showImguiDemo);
    }
}

} // namespace

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "ぺらぺらアニメ作り機 - Phase 0",
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

    perapera::ui::applyDarkTheme();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    bool running = true;
    bool showImguiDemo = true;
    AppMode currentMode = AppMode::Project;

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

        drawAppUi(currentMode, showImguiDemo);

        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 13, 13, 16, 255);
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

    return 0;
}
