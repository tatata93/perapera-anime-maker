cmake_minimum_required(VERSION 3.26)

project(perapera_anime_maker
    VERSION 0.1.0
    DESCRIPTION "Hand-drawn 2D animation tool"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# exe を build/bin にまとめる。
# ユーザーの起動確認コマンドが build 以下から exe を探せるようにする。
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# SDL3: ウィンドウ・入力・Renderer を担当する。
# zlib ライセンス。
set(SDL_SHARED ON CACHE BOOL "Build SDL3 as a shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Do not build SDL3 as a static library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Do not build SDL3 test library" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.0
    GIT_SHALLOW TRUE
)

# Dear ImGui: UIパネルを担当する。
# MIT ライセンス。
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
    GIT_SHALLOW TRUE
)

# nlohmann/json: project.json と layer_NNN.json 保存で使う。
# MIT ライセンス。
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 imgui nlohmann_json)

add_library(imgui_sdl3_renderer STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(imgui_sdl3_renderer PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui_sdl3_renderer PUBLIC SDL3::SDL3)

add_executable(perapera_anime_maker
    src/app/main.cpp

    src/core/Stroke.cpp
    src/core/Layer.cpp
    src/core/Frame.cpp
    src/core/Cell.cpp
    src/core/Project.cpp

    src/io/ProjectIO.cpp
    src/io/PngExporter.cpp
    src/io/FfmpegRunner.cpp

    src/render/CanvasBitmap.cpp
    src/render/CanvasRenderer.cpp

    src/brush/SimpleBrushEngine.cpp

    src/camera/ShotCamera2D.cpp
    src/camera/CameraAnimation.cpp

    src/ui/App.cpp
    src/ui/AppDrawingMode.cpp
    src/ui/AppInput.cpp
    src/ui/AppLightTable.cpp
    src/ui/AppOperations.cpp
    src/ui/AppProjectIO.cpp
    src/ui/AppProjectIOSupport.cpp
    src/ui/CanvasView.cpp
    src/ui/panels/BrushPanel.cpp
    src/ui/panels/LayerPanel.cpp
    src/ui/panels/FramePanel.cpp
    src/ui/panels/TimelinePanel.cpp
    src/ui/panels/ExportPanel.cpp
)

target_include_directories(perapera_anime_maker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(perapera_anime_maker PRIVATE
    SDL3::SDL3
    imgui_sdl3_renderer
    nlohmann_json::nlohmann_json
)

if (MSVC)
    target_compile_options(perapera_anime_maker PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_anime_maker PRIVATE -Wall -Wextra -Wpedantic)
endif()

# WindowsでSDL3.dllをexe横にコピーする。
# SDL3を共有ライブラリでFetchContentビルドした場合、これがないと起動時にDLLが見つからない。
add_custom_command(TARGET perapera_anime_maker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:perapera_anime_maker>
)
