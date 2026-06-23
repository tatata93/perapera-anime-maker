// このファイルの役割:
// アプリのエントリーポイント。
// Phase 0 では SDL3 のウィンドウ作成、Dear ImGui 初期化、
// モードタブの切り替え骨格だけを持つ。
// 作画データやレンダリングコアは Phase 1 以降で別ファイルに分離する。
//
// 日本語表示の注意:
// C++ソースの文字コードやWindowsコードページに依存しないよう、
// UIに出す日本語文字列は \uXXXX エスケープ付きの u8 文字列にしている。
// フォントは同梱せず、Windows標準の msgothic.ttc を使う。

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "ui/Theme.h"

namespace {

constexpr int kInitialWindowWidth = 1280;
constexpr int kInitialWindowHeight = 800;

// C++20 では u8"..." の型が const char8_t* になる。
// SDL3 と Dear ImGui は UTF-8 の const char* を受け取るため、
// ここで明示的にバイト列として渡す。
const char* u8c(const char8_t* text)
{
    return reinterpret_cast<const char*>(text);
}

struct FontLoadStatus {
    bool loaded = false;
    std::string path;
    int fontNo = -1;
    std::string message = "not tried";
};

FontLoadStatus gJapaneseFontStatus;
ImVector<ImWchar> gJapaneseGlyphRanges;

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

const std::array<ModeInfo, 5> kModes = {{
    {u8c(u8"1 \u30D7\u30ED\u30B8\u30A7\u30AF\u30C8"), u8c(u8"\u30D7\u30ED\u30B8\u30A7\u30AF\u30C8\u30E2\u30FC\u30C9"), u8c(u8"\u4F5C\u54C1\u3068\u30AB\u30C3\u30C8\u3092\u7BA1\u7406\u3059\u308B\u5834\u6240\u3002Phase 0 \u3067\u306F\u7A7A\u30D1\u30CD\u30EB\u306E\u307F\u3002")},
    {u8c(u8"2 \u7D75\u30B3\u30F3\u30C6"), u8c(u8"\u7D75\u30B3\u30F3\u30C6\u30E2\u30FC\u30C9"), u8c(u8"\u7D75\u30B3\u30F3\u30C6\u3092\u63CF\u304D\u3001\u53F0\u8A5E\u30FB\u30A2\u30AF\u30B7\u30E7\u30F3\u30FB\u30AB\u30E1\u30E9\u6307\u793A\u3092\u66F8\u304F\u5834\u6240\u3002Phase 0 \u3067\u306F\u7A7A\u30D1\u30CD\u30EB\u306E\u307F\u3002")},
    {u8c(u8"2.5 \u30D7\u30EA\u30D3\u30BA"), u8c(u8"\u30D7\u30EA\u30D3\u30BA\u30E2\u30FC\u30C9"), u8c(u8"3D\u4E0B\u6577\u304D\u3084\u4F5C\u753B\u88DC\u52A9\u3092\u6271\u3046\u5834\u6240\u3002Phase 2.5 \u3067\u672C\u5B9F\u88C5\u3059\u308B\u3002")},
    {u8c(u8"3 \u4F5C\u753B"), u8c(u8"\u4F5C\u753B\u30E2\u30FC\u30C9"), u8c(u8"\u30D6\u30E9\u30B7\u3001\u30EC\u30A4\u30E4\u30FC\u3001\u30D5\u30EC\u30FC\u30E0\u3001\u30BF\u30A4\u30E0\u30E9\u30A4\u30F3\u3092\u4F7F\u3063\u3066\u4F5C\u753B\u3059\u308B\u5834\u6240\u3002Phase 1 \u3067\u672C\u5B9F\u88C5\u3059\u308B\u3002")},
    {u8c(u8"5 \u51FA\u529B"), u8c(u8"\u51FA\u529B\u30E2\u30FC\u30C9"), u8c(u8"PNG\u9023\u756A\u3084MP4\u3092\u66F8\u304D\u51FA\u3059\u5834\u6240\u3002Phase 1 \u3067\u6700\u4F4E\u9650\u306E\u66F8\u304D\u51FA\u3057\u3092\u8FFD\u52A0\u3059\u308B\u3002")},
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

std::vector<std::string> buildJapaneseFontCandidatePaths()
{
    // まずWindows標準のMS Gothicを固定で試す。
    // msgothic.ttc は日本語Windowsに標準で入り、ImGui/FreeTypeなしでも読みやすい。
    return {
        "C:/Windows/Fonts/msgothic.ttc",
        "C:/Windows/Fonts/meiryo.ttc",
        "C:/Windows/Fonts/YuGothM.ttc",
        "C:/Windows/Fonts/YuGothR.ttc",
        "C:/Windows/Fonts/BIZ-UDGothicR.ttc",
    };
}

void addPhase0JapaneseText(ImFontGlyphRangesBuilder& builder)
{
    // Fix6:
    // CJK全域やImGuiの大きな日本語既定範囲は使わない。
    // SDL_Renderer3で巨大なフォントテクスチャが作られて起動失敗する可能性を避けるため、
    // Phase 0で実際に画面に出す文字だけを最小限で登録する。
    builder.AddText(u8c(u8"1 \u30D7\u30ED\u30B8\u30A7\u30AF\u30C8"));
    builder.AddText(u8c(u8"2 \u7D75\u30B3\u30F3\u30C6"));
    builder.AddText(u8c(u8"2.5 \u30D7\u30EA\u30D3\u30BA"));
    builder.AddText(u8c(u8"3 \u4F5C\u753B"));
    builder.AddText(u8c(u8"5 \u51FA\u529B"));
    builder.AddText(u8c(u8"\u30D7\u30ED\u30B8\u30A7\u30AF\u30C8\u30E2\u30FC\u30C9\u7D75\u30B3\u30F3\u30C6\u30D7\u30EA\u30D3\u30BA\u4F5C\u753B\u51FA\u529B"));
    builder.AddText(u8c(u8"\u4F5C\u54C1\u3068\u30AB\u30C3\u30C8\u3092\u7BA1\u7406\u3059\u308B\u5834\u6240\u3002\u3067\u306F\u7A7A\u30D1\u30CD\u30EB\u306E\u307F"));
    builder.AddText(u8c(u8"\u63CF\u304D\u53F0\u8A5E\u30FB\u30A2\u30AF\u30B7\u30E7\u30F3\u30FB\u30AB\u30E1\u30E9\u6307\u793A\u3092\u66F8\u304F"));
    builder.AddText(u8c(u8"3D\u4E0B\u6577\u304D\u3084\u4F5C\u753B\u88DC\u52A9\u3092\u6271\u3046\u672C\u5B9F\u88C5"));
    builder.AddText(u8c(u8"\u30D6\u30E9\u30B7\u3001\u30EC\u30A4\u30E4\u30FC\u3001\u30D5\u30EC\u30FC\u30E0\u3001\u30BF\u30A4\u30E0\u30E9\u30A4\u30F3\u3092\u4F7F\u3063\u3066\u3059\u308B"));
    builder.AddText(u8c(u8"PNG\u9023\u756A\u3084MP4\u3092\u66F8\u304D\u51FA\u3059\u6700\u4F4E\u9650\u306E\u8FFD\u52A0"));
    builder.AddText(u8c(u8"\u30D5\u30A1\u30A4\u30EB\u7DE8\u96C6\u8868\u793A\u30A6\u30A3\u30F3\u30C9\u30A6\u30D8\u30EB\u30D7\u65B0\u898F\u958B\u304F\u4FDD\u5B58\u5225\u540D\u4FDD\u5B58"));
    builder.AddText(u8c(u8"\u30BA\u30FC\u30E0\u30A4\u30F3\u30BA\u30FC\u30E0\u30A2\u30A6\u30C8\u5168\u4F53\u8868\u793A\u30C7\u30E2"));
    builder.AddText(u8c(u8"\u307A\u3089\u307A\u3089\u30A2\u30CB\u30E1\u4F5C\u308A\u6A5F"));
    builder.AddText(u8c(u8"\u30EC\u30A4\u30A2\u30A6\u30C8\u30EA\u30BB\u30C3\u30C8\u3053\u306E\u30E2\u30FC\u30C9\u3092\u5168\u30E2\u30FC\u30C9\u5225\u30E2\u30FC\u30C9\u306E\u8AAD\u307F\u8FBC\u3080"));
    builder.AddText(u8c(u8"\u65E5\u672C\u8A9E\u30C6\u30B9\u30C8\uFF1A\u4F5C\u753B\u7D75\u30B3\u30F3\u30C6\u30BF\u30A4\u30E0\u30E9\u30A4\u30F3"));
    builder.AddText(u8c(u8"\u5DE6\u30B5\u30A4\u30C9\u30D0\u30FC\u53F3\u30B5\u30A4\u30C9\u30D0\u30FC\u30E1\u30A4\u30F3\u30AD\u30E3\u30F3\u30D0\u30B9"));
    builder.AddText(u8c(u8"\u67A0\u3060\u3051\u30C4\u30FC\u30EB\u8A2D\u5B9A\u30AB\u30E9\u30FC\u30AA\u30CB\u30AA\u30F3\u30B9\u30AD\u30F3\u60C5\u5831\u4E00\u89A7"));
    builder.AddText(u8c(u8"\u518D\u751F\u30B3\u30F3\u30C8\u30ED\u30FC\u30EB\u30D7\u30EC\u30A4\u30D8\u30C3\u30C9"));
}

void buildJapaneseGlyphRanges(ImGuiIO& io)
{
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    addPhase0JapaneseText(builder);
    builder.BuildRanges(&gJapaneseGlyphRanges);
}

void setupJapaneseFont(ImGuiIO& io)
{
    // Fix6:
    // アプリが起動しない問題を最優先で避ける。
    // フォントアトラスを巨大化させず、Phase 0で使う文字だけを登録する。
    io.Fonts->Clear();
    buildJapaneseGlyphRanges(io);

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
            gJapaneseFontStatus.loaded = true;
            gJapaneseFontStatus.path = fontPath;
            gJapaneseFontStatus.fontNo = fontNo;
            gJapaneseFontStatus.message = "loaded minimal Japanese glyph set / Fix6";
            std::fprintf(stderr, "Japanese UI font loaded: %s fontNo=%d / Fix6\n", fontPath.c_str(), fontNo);
            return;
        }
    }

    io.Fonts->AddFontDefault();
    io.FontDefault = nullptr;

    gJapaneseFontStatus.loaded = false;
    gJapaneseFontStatus.path = "none";
    gJapaneseFontStatus.fontNo = -1;
    gJapaneseFontStatus.message = "Japanese font file was not loaded; fallback font used / Fix6";
    std::fprintf(stderr, "Japanese UI font was not loaded. Fallback font used. / Fix6\n");
}

void drawMainMenuBar(bool& showImguiDemo)
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu(u8c(u8"\u30D5\u30A1\u30A4\u30EB"))) {
        ImGui::MenuItem(u8c(u8"\u65B0\u898F"), nullptr, false, false);
        ImGui::MenuItem(u8c(u8"\u958B\u304F"), nullptr, false, false);
        ImGui::MenuItem(u8c(u8"\u4FDD\u5B58"), "Ctrl+S", false, false);
        ImGui::MenuItem(u8c(u8"\u5225\u540D\u4FDD\u5B58"), nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(u8c(u8"\u7DE8\u96C6"))) {
        ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
        ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(u8c(u8"\u8868\u793A"))) {
        ImGui::MenuItem(u8c(u8"\u30BA\u30FC\u30E0\u30A4\u30F3"), nullptr, false, false);
        ImGui::MenuItem(u8c(u8"\u30BA\u30FC\u30E0\u30A2\u30A6\u30C8"), nullptr, false, false);
        ImGui::MenuItem(u8c(u8"\u5168\u4F53\u8868\u793A"), "F", false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(u8c(u8"\u30A6\u30A3\u30F3\u30C9\u30A6"))) {
        ImGui::MenuItem(u8c(u8"ImGui\u30C7\u30E2"), nullptr, &showImguiDemo);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(u8c(u8"\u30D8\u30EB\u30D7"))) {
        ImGui::TextUnformatted(u8c(u8"\u307A\u3089\u307A\u3089\u30A2\u30CB\u30E1\u4F5C\u308A\u6A5F Phase 0"));
        ImGui::TextDisabled("Shortcut list will be added in a later phase.");
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
    if (ImGui::Button(u8c(u8"\u30EC\u30A4\u30A2\u30A6\u30C8\u30EA\u30BB\u30C3\u30C8"), ImVec2(155.0f, 28.0f))) {
        ImGui::OpenPopup("LayoutResetPopup");
    }

    if (ImGui::BeginPopup("LayoutResetPopup")) {
        ImGui::MenuItem(u8c(u8"\u3053\u306E\u30E2\u30FC\u30C9\u3092\u30EA\u30BB\u30C3\u30C8"), nullptr, false, false);
        ImGui::MenuItem(u8c(u8"\u5168\u30E2\u30FC\u30C9\u30EA\u30BB\u30C3\u30C8"), nullptr, false, false);
        ImGui::MenuItem(u8c(u8"\u5225\u30E2\u30FC\u30C9\u306E\u30EC\u30A4\u30A2\u30A6\u30C8\u3092\u8AAD\u307F\u8FBC\u3080"), nullptr, false, false);
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

    ImGui::Text("JP font: %s", gJapaneseFontStatus.loaded ? "OK" : "NG");
    ImGui::Text("JP font file: %s", gJapaneseFontStatus.path.c_str());
    ImGui::Text("JP font index: %d", gJapaneseFontStatus.fontNo);
    ImGui::Text("JP font message: %s", gJapaneseFontStatus.message.c_str());
    ImGui::TextUnformatted("JP sample below should be Japanese / Fix6 Japanese font:");
    ImGui::TextUnformatted(u8c(u8"\u65E5\u672C\u8A9E\u30C6\u30B9\u30C8\uFF1A\u307A\u3089\u307A\u3089\u30A2\u30CB\u30E1\u4F5C\u308A\u6A5F / \u4F5C\u753B / \u7D75\u30B3\u30F3\u30C6 / \u30BF\u30A4\u30E0\u30E9\u30A4\u30F3"));
    ImGui::Separator();

    const float sideWidth = 240.0f;
    const float rightWidth = 280.0f;
    const float timelineHeight = 110.0f;
    const float availableHeight = ImGui::GetContentRegionAvail().y;
    const float centerHeight = availableHeight - timelineHeight - ImGui::GetStyle().ItemSpacing.y;

    ImGui::BeginChild("UpperArea", ImVec2(0.0f, centerHeight), false);

    ImGui::BeginChild("LeftSidebar", ImVec2(sideWidth, 0.0f), true);
    drawPlaceholderPanel(u8c(u8"\u5DE6\u30B5\u30A4\u30C9\u30D0\u30FC"), u8c(u8"Phase 0\u3067\u306F\u67A0\u3060\u3051\u3002Phase 1\u3067\u30C4\u30FC\u30EB\u3001\u30D6\u30E9\u30B7\u8A2D\u5B9A\u3001\u30AB\u30E9\u30FC\u3001\u30AA\u30CB\u30AA\u30F3\u30B9\u30AD\u30F3\u3092\u5165\u308C\u308B\u3002"));
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
    ImGui::TextUnformatted(u8c(u8"\u30E1\u30A4\u30F3\u30AD\u30E3\u30F3\u30D0\u30B9"));
    ImGui::TextDisabled("CanvasRenderer will be connected in Phase 1.");
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightSidebar", ImVec2(rightWidth, 0.0f), true);
    drawPlaceholderPanel(u8c(u8"\u53F3\u30B5\u30A4\u30C9\u30D0\u30FC"), u8c(u8"Phase 1\u3067\u30EC\u30A4\u30E4\u30FC\u3001\u30D5\u30EC\u30FC\u30E0\u60C5\u5831\u3001\u66F8\u304D\u51FA\u3057\u30D1\u30CD\u30EB\u3092\u5165\u308C\u308B\u3002Phase 2\u3067\u30BB\u30EB\u4E00\u89A7\u3092\u8FFD\u52A0\u3059\u308B\u3002"));
    ImGui::EndChild();

    ImGui::EndChild();

    drawPlaceholderPanel(u8c(u8"\u30BF\u30A4\u30E0\u30E9\u30A4\u30F3"), u8c(u8"Phase 0\u3067\u306F\u67A0\u3060\u3051\u3002Phase 1\u3067\u30D5\u30EC\u30FC\u30E0\u8FFD\u52A0\u3001\u518D\u751F\u30B3\u30F3\u30C8\u30ED\u30FC\u30EB\u3001\u30D7\u30EC\u30A4\u30D8\u30C3\u30C9\u3092\u5165\u308C\u308B\u3002"));

    ImGui::EndChild();
}

void drawStatusBar()
{
    ImGui::BeginChild("StatusBar", ImVec2(0.0f, 24.0f), true);
    ImGui::Text("fps: -- | canvas: -- | zoom: 100%% | brush: -- px | JP font: %s", gJapaneseFontStatus.loaded ? "OK" : "NG");
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
        u8c(u8"\u307A\u3089\u307A\u3089\u30A2\u30CB\u30E1\u4F5C\u308A\u6A5F - Phase 0"),
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
