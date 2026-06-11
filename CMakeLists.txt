cmake_minimum_required(VERSION 3.22)

# ぺらぺらアニメ作り機
# C++で手描き2Dアニメ制作ソフトを作るためのプロジェクト。
#
# Phase 2では、任意サイズ作画キャンバスと撮影フレームの土台を追加する。
# まだ本格的な作画機能は入れず、データ構造と仮プレビューを作る。

project(PeraperaAnimeMaker
    VERSION 0.2.0
    LANGUAGES CXX
)

# C++20を使う。
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# FetchContentは、CMake実行時に外部ライブラリを取得するための仕組み。
include(FetchContent)

# ------------------------------------------------------------
# SDL3
# ------------------------------------------------------------
# SDL3はウィンドウ作成、入力処理、レンダラー作成に使う。
# ライセンスはzlibで、安全寄りの構成として採用する。
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
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 imgui)

# Dear ImGui本体とSDL3用バックエンドをまとめた静的ライブラリを作る。
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp

    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# ------------------------------------------------------------
# ぺらぺらアニメ作り機 本体
# ------------------------------------------------------------

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

add_executable(perapera_anime_maker
    src/app/main.cpp

    src/drawing/WorkCanvas.h
    src/drawing/WorkCanvas.cpp

    src/project/RenderFormat.h
    src/project/RenderFormat.cpp

    src/ui/CanvasPreview.h
    src/ui/CanvasPreview.cpp
)

# 自作ヘッダーを src/ から include できるようにする。
target_include_directories(perapera_anime_maker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(perapera_anime_maker PRIVATE
    SDL3::SDL3-static
    imgui
)

if(MSVC)
    target_compile_options(perapera_anime_maker PRIVATE /W4 /utf-8)
else()
    target_compile_options(perapera_anime_maker PRIVATE -Wall -Wextra -Wpedantic)
endif()