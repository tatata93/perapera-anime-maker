cmake_minimum_required(VERSION 3.22)

project(PeraperaAnimeMaker VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "Build SDL3 as shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL3 as static library" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.10
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 imgui)

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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

add_executable(perapera_anime_maker
    src/app/main.cpp
    src/camera/ShotCamera2D.h
    src/camera/ShotCamera2D.cpp

    src/drawing/Brush.h
    src/drawing/Brush.cpp
    src/drawing/Stroke.h
    src/drawing/Stroke.cpp
    src/drawing/DrawingLayer.h
    src/drawing/DrawingLayer.cpp
    src/drawing/AnimationFrame.h
    src/drawing/AnimationFrame.cpp
    src/drawing/WorkCanvas.h
    src/drawing/WorkCanvas.cpp

    src/project/RenderFormat.h
    src/project/RenderFormat.cpp

    src/export/PngExporter.h
    src/export/PngExporter.cpp

    src/ui/CanvasPreview.h
    src/ui/CanvasPreview.cpp
    src/ui/DrawingCanvasPanel.h
    src/ui/DrawingCanvasPanel.cpp
)

target_include_directories(perapera_anime_maker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
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
