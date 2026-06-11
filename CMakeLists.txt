cmake_minimum_required(VERSION 3.22)

# ぺらぺらアニメ作り機
# C++で手描き2Dアニメ制作ソフトを作るためのプロジェクト。
#
# Phase 1では SDL3 と Dear ImGui を追加する。
# SDL3はウィンドウ作成と入力処理に使う。
# Dear ImGuiは開発初期のGUI、メニュー、インスペクタに使う。
#
# 注意:
# アプリの正式名称は日本語の「ぺらぺらアニメ作り機」だが、
# CMakeプロジェクト名や実行ファイル名は、環境差による文字化けや
# パス問題を避けるため、ASCII名の PeraperaAnimeMaker を使う。

project(PeraperaAnimeMaker
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++20を使う。
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# FetchContentは、CMake実行時に外部ライブラリを取得するための仕組み。
# ここではSDL3とDear ImGuiをGitHubから取得する。
include(FetchContent)

# ------------------------------------------------------------
# SDL3
# ------------------------------------------------------------
# SDL3はウィンドウ作成、入力処理、レンダラー作成に使う。
# ライセンスはzlibで、安全寄りの構成として採用する。
#
# SDLを静的リンクに寄せることで、初期段階ではDLL配置の混乱を減らす。
# zlibライセンスなので、静的リンクしてもGPLのような強いコピーレフトはない。
set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL3 as static library" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.10
    GIT_SHALLOW TRUE
)

# ------------------------------------------------------------
# Dear ImGui
# ------------------------------------------------------------
# Dear ImGuiは、開発初期のGUIを作るために使う。
# 公式にはCMakeプロジェクトが用意されていないため、
# 必要なcppファイルをこちらのCMakeLists.txtで指定してライブラリ化する。
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
    GIT_SHALLOW TRUE
)

# SDL3とDear ImGuiを取得・展開する。
FetchContent_MakeAvailable(SDL3 imgui)

# Dear ImGui本体とSDL3用バックエンドをまとめた静的ライブラリを作る。
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp

    # SDL3から入力イベントを受け取るためのバックエンド。
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp

    # SDL3のRendererでDear ImGuiを描画するためのバックエンド。
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

# Dear ImGuiのヘッダー検索パスを設定する。
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Dear ImGuiのSDL3バックエンドはSDL3のヘッダーを使うのでリンクする。
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# ------------------------------------------------------------
# ぺらぺらアニメ作り機 本体
# ------------------------------------------------------------

# 実行ファイルの出力先を build/bin にまとめる。
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# 実行ファイルを作る。
# 出力名は日本語ではなく perapera_anime_maker にする。
add_executable(perapera_anime_maker
    src/app/main.cpp
)

# SDL3とDear ImGuiを本体にリンクする。
target_link_libraries(perapera_anime_maker PRIVATE
    SDL3::SDL3-static
    imgui
)

# コンパイラごとの警告設定。
if(MSVC)
    # /W4 は警告を多めに出す設定。
    # /utf-8 はソースコード内の日本語文字列をUTF-8として扱うための設定。
    target_compile_options(perapera_anime_maker PRIVATE /W4 /utf-8)
else()
    target_compile_options(perapera_anime_maker PRIVATE -Wall -Wextra -Wpedantic)
endif()