cmake_minimum_required(VERSION 3.24)

project(perapera_anime_maker
    VERSION 0.1.0
    DESCRIPTION "Hand-drawn 2D animation tool"
    LANGUAGES CXX
)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put the executable in build/bin so the user can find it easily.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# SDL3 is used for the window, input events, and the Phase 1 renderer path.
# Tests are not needed for this application build.
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
    GIT_SHALLOW TRUE
)

# Dear ImGui provides all editor panels.
# Dear ImGui does not require its own CMake project here; we compile the needed
# source files into imgui_backend below.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE
)

# nlohmann/json is used from Phase 1 for project files.
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

FetchContent_MakeAvailable(SDL3 nlohmann_json)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

add_library(imgui_backend STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
)

target_include_directories(imgui_backend PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
)

target_link_libraries(imgui_backend PUBLIC SDL3::SDL3)

add_executable(perapera_anime_maker
    src/app/main.cpp
    src/core/Stroke.cpp
    src/core/Layer.cpp
    src/core/Frame.cpp
    src/core/Cell.cpp
    src/core/Project.cpp
    src/io/ProjectIO.cpp
    src/render/CanvasBitmap.cpp
    src/render/CanvasRenderer.cpp
    src/brush/SimpleBrushEngine.cpp
    src/camera/ShotCamera2D.cpp
    src/camera/CameraAnimation.cpp
)

target_include_directories(perapera_anime_maker PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(perapera_anime_maker PRIVATE
    SDL3::SDL3
    imgui_backend
    nlohmann_json::nlohmann_json
)

if(MSVC)
    target_compile_options(perapera_anime_maker PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(imgui_backend PRIVATE /W3 /utf-8)
else()
    target_compile_options(perapera_anime_maker PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy the SDL runtime DLL next to the executable on Windows.
# This keeps build/bin/perapera_anime_maker.exe directly runnable.
if(WIN32)
    add_custom_command(TARGET perapera_anime_maker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:perapera_anime_maker>
        VERBATIM
    )
endif()
