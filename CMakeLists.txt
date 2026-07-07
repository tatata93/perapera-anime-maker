cmake_minimum_required(VERSION 3.26)

project(
    perapera_anime_maker
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

# libmypaint: Phase 1.5で導入するブラシエンジンライブラリ。
# vcpkgのlibmypaint portはpkg-config情報だけを持つ構成がある。
# さらにVisual Studio同梱vcpkgではpkg-config検出が環境変数に左右されやすいため、
# 1) CMake config
# 2) pkg-config
# 3) vcpkg_installed/ 直下のヘッダ/ライブラリ直接検出
# の順で安全に探す。
find_package(libmypaint CONFIG QUIET)
find_package(PkgConfig QUIET)

if (PkgConfig_FOUND)
    pkg_check_modules(LIBMYPAINT QUIET IMPORTED_TARGET libmypaint)
    if (NOT LIBMYPAINT_FOUND)
        pkg_check_modules(LIBMYPAINT QUIET IMPORTED_TARGET libmypaint-1.6)
    endif()
    if (NOT LIBMYPAINT_FOUND)
        pkg_check_modules(LIBMYPAINT QUIET IMPORTED_TARGET libmypaint-2.0)
    endif()
endif()

set(PERAPERA_LIBMYPAINT_AVAILABLE OFF)
set(PERAPERA_LIBMYPAINT_DIRECT OFF)
set(PERAPERA_LIBMYPAINT_DIRECT_INCLUDE_DIRS "")
set(PERAPERA_LIBMYPAINT_DIRECT_LIBRARY "")
set(PERAPERA_VCPKG_RUNTIME_BIN_DIR "")

if (libmypaint_FOUND)
    set(PERAPERA_LIBMYPAINT_AVAILABLE ON)
    message(STATUS "libmypaint found by CMake config")
elseif (LIBMYPAINT_FOUND)
    set(PERAPERA_LIBMYPAINT_AVAILABLE ON)
    message(STATUS "libmypaint found by pkg-config: ${LIBMYPAINT_VERSION}")
else()
    # vcpkg manifest modeでは、インストール先がプロジェクト直下の
    # vcpkg_installed/ になる。pkg-configが拾えない場合でも、
    # この場所に libmypaint のヘッダと import lib があれば使える。
    if (DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET STREQUAL "")
        set(PERAPERA_VCPKG_TRIPLET "${VCPKG_TARGET_TRIPLET}")
    else()
        set(PERAPERA_VCPKG_TRIPLET "x64-windows")
    endif()

    set(PERAPERA_VCPKG_TRIPLET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${PERAPERA_VCPKG_TRIPLET}")
    set(PERAPERA_LIBMYPAINT_HEADER "${PERAPERA_VCPKG_TRIPLET_DIR}/include/libmypaint/mypaint-brush.h")
    set(PERAPERA_LIBMYPAINT_IMPORT_LIB "${PERAPERA_VCPKG_TRIPLET_DIR}/lib/mypaint.lib")

    if (EXISTS "${PERAPERA_LIBMYPAINT_HEADER}" AND EXISTS "${PERAPERA_LIBMYPAINT_IMPORT_LIB}")
        set(PERAPERA_LIBMYPAINT_AVAILABLE ON)
        set(PERAPERA_LIBMYPAINT_DIRECT ON)
        set(PERAPERA_LIBMYPAINT_DIRECT_LIBRARY "${PERAPERA_LIBMYPAINT_IMPORT_LIB}")
        set(PERAPERA_VCPKG_RUNTIME_BIN_DIR "${PERAPERA_VCPKG_TRIPLET_DIR}/bin")
        set(PERAPERA_LIBMYPAINT_DIRECT_INCLUDE_DIRS
            "${PERAPERA_VCPKG_TRIPLET_DIR}/include"
            "${PERAPERA_VCPKG_TRIPLET_DIR}/include/libmypaint"
            "${PERAPERA_VCPKG_TRIPLET_DIR}/include/glib-2.0"
            "${PERAPERA_VCPKG_TRIPLET_DIR}/lib/glib-2.0/include"
        )
        message(STATUS "libmypaint found by vcpkg_installed direct path: ${PERAPERA_VCPKG_TRIPLET_DIR}")
    else()
        message(STATUS "libmypaint not found: MyPaintBrushEngine will use Simple fallback")
    endif()
endif()

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
    src/core/Timesheet.cpp
    src/core/TimesheetResolver.cpp
    src/core/TimesheetSceneResolver.cpp
    src/core/TimesheetInbetweenResolver.cpp
    src/core/TimesheetInbetweenPlanner.cpp
    src/io/ProjectIO.cpp
    src/io/TimesheetIO.cpp
    src/io/CutIO.cpp
    src/io/SceneCutLayoutIO.cpp
    src/io/LayerLayoutIO.cpp
    src/io/ProjectLayoutSaveEntry.cpp
    src/io/ProjectLayoutReadEntry.cpp
    src/io/PngExporter.cpp
    src/io/FfmpegRunner.cpp
    src/render/CanvasBitmap.cpp
    src/render/CanvasRenderer.cpp
    src/brush/SimpleBrushEngine.cpp
    src/brush/MyPaintBrushEngine.cpp
    src/fill/FloodFill.cpp
    src/camera/ShotCamera2D.cpp
    src/camera/CameraAnimation.cpp
    src/ui/App.cpp
    src/ui/AppDrawingMode.cpp
    src/ui/AppInput.cpp
    src/ui/AppLightTable.cpp
    src/ui/AppOperations.cpp
    src/ui/AppProjectIO.cpp
    src/ui/AppPaletteIO.cpp
    src/ui/AppProjectIOSupport.cpp
    src/ui/CanvasView.cpp
    src/ui/panels/BrushPanel.cpp
    src/ui/panels/CellPanel.cpp
    src/ui/panels/TimesheetPanel.cpp
    src/ui/panels/TimesheetPanelBridge.cpp
    src/ui/panels/ColorPanel.cpp
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

if (PERAPERA_LIBMYPAINT_AVAILABLE)
    target_compile_definitions(perapera_anime_maker PRIVATE PERAPERA_HAS_LIBMYPAINT=1)

    if (TARGET libmypaint::libmypaint)
        target_link_libraries(perapera_anime_maker PRIVATE libmypaint::libmypaint)
    elseif (TARGET libmypaint)
        target_link_libraries(perapera_anime_maker PRIVATE libmypaint)
    elseif (TARGET PkgConfig::LIBMYPAINT)
        target_link_libraries(perapera_anime_maker PRIVATE PkgConfig::LIBMYPAINT)
    elseif (PERAPERA_LIBMYPAINT_DIRECT)
        target_include_directories(perapera_anime_maker PRIVATE
            ${PERAPERA_LIBMYPAINT_DIRECT_INCLUDE_DIRS}
        )
        target_link_libraries(perapera_anime_maker PRIVATE
            "${PERAPERA_LIBMYPAINT_DIRECT_LIBRARY}"
        )
    else()
        target_include_directories(perapera_anime_maker PRIVATE ${LIBMYPAINT_INCLUDE_DIRS})
        target_link_directories(perapera_anime_maker PRIVATE ${LIBMYPAINT_LIBRARY_DIRS})
        target_link_libraries(perapera_anime_maker PRIVATE ${LIBMYPAINT_LIBRARIES})
    endif()
endif()

add_executable(perapera_timesheet_resolver_selftest
    tools/timesheet_resolver_selftest.cpp
    src/core/Timesheet.cpp
    src/core/TimesheetResolver.cpp
)
target_include_directories(perapera_timesheet_resolver_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(perapera_timesheet_io_selftest
    tools/timesheet_io_selftest.cpp
    src/core/Timesheet.cpp
    src/core/TimesheetResolver.cpp
    src/io/TimesheetIO.cpp
)
target_include_directories(perapera_timesheet_io_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(perapera_timesheet_io_selftest PRIVATE nlohmann_json::nlohmann_json)

add_executable(perapera_cut_io_selftest
    tools/cut_io_selftest.cpp
    src/core/Timesheet.cpp
    src/core/TimesheetResolver.cpp
    src/io/TimesheetIO.cpp
    src/io/CutIO.cpp
)
target_include_directories(perapera_cut_io_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(perapera_cut_io_selftest PRIVATE nlohmann_json::nlohmann_json)

add_executable(perapera_timesheet_scene_resolver_selftest
    tools/timesheet_scene_resolver_selftest.cpp
    src/core/Timesheet.cpp
    src/core/TimesheetResolver.cpp
    src/core/TimesheetSceneResolver.cpp
)
target_include_directories(perapera_timesheet_scene_resolver_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(perapera_timesheet_inbetween_planner_selftest
    tools/timesheet_inbetween_planner_selftest.cpp
    src/core/Timesheet.cpp
    src/core/TimesheetInbetweenPlanner.cpp
)
target_include_directories(perapera_timesheet_inbetween_planner_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(perapera_timesheet_inbetween_resolver_selftest
    tools/timesheet_inbetween_resolver_selftest.cpp
    src/core/Timesheet.cpp
    src/core/TimesheetInbetweenResolver.cpp
)
target_include_directories(perapera_timesheet_inbetween_resolver_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)


add_executable(perapera_project_active_cut_bridge_selftest
    tools/project_active_cut_bridge_selftest.cpp
)
target_include_directories(perapera_project_active_cut_bridge_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_executable(perapera_timesheet_panel_bridge_selftest
    tools/timesheet_panel_bridge_selftest.cpp
    src/core/Timesheet.cpp
    src/ui/panels/TimesheetPanelBridge.cpp
)
target_include_directories(perapera_timesheet_panel_bridge_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)


add_executable(perapera_cell_timesheet_alignment_selftest
    tools/cell_timesheet_alignment_selftest.cpp
    src/core/Timesheet.cpp
)

target_include_directories(perapera_cell_timesheet_alignment_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# final_spec_v6 Phase 2 Step 1-a: Scene / active Cut minimal model selftest.
add_executable(perapera_scene_cut_model_selftest
    tools/scene_cut_model_selftest.cpp
    src/core/Timesheet.cpp
)
target_include_directories(perapera_scene_cut_model_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if (MSVC)
    target_compile_options(perapera_scene_cut_model_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_scene_cut_model_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (MSVC)
    target_compile_options(perapera_anime_maker PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(perapera_timesheet_resolver_selftest PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(perapera_timesheet_io_selftest PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(perapera_cut_io_selftest PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(perapera_timesheet_scene_resolver_selftest PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(perapera_timesheet_inbetween_planner_selftest PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(perapera_timesheet_inbetween_resolver_selftest PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(perapera_timesheet_panel_bridge_selftest PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(perapera_project_active_cut_bridge_selftest PRIVATE /W4 /permissive- /utf-8)

 target_compile_options(perapera_cell_timesheet_alignment_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_anime_maker PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(perapera_timesheet_resolver_selftest PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(perapera_timesheet_io_selftest PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(perapera_cut_io_selftest PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(perapera_timesheet_scene_resolver_selftest PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(perapera_timesheet_inbetween_planner_selftest PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(perapera_timesheet_inbetween_resolver_selftest PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(perapera_timesheet_panel_bridge_selftest PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(perapera_project_active_cut_bridge_selftest PRIVATE -Wall -Wextra -Wpedantic)

 target_compile_options(perapera_cell_timesheet_alignment_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# WindowsでSDL3.dllをexe横にコピーする。
# SDL3を共有ライブラリでFetchContentビルドした場合、これがないと起動時にDLLが見つからない。
add_custom_command(TARGET perapera_anime_maker POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:perapera_anime_maker>
)

# vcpkg直下からlibmypaintを直接拾った場合、実行時に必要なDLLもexe横へコピーする。
# vcpkg_installed はgit管理しない。ローカル環境の依存物として扱う。
if (PERAPERA_LIBMYPAINT_DIRECT AND EXISTS "${PERAPERA_VCPKG_RUNTIME_BIN_DIR}")
    add_custom_command(TARGET perapera_anime_maker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PERAPERA_VCPKG_RUNTIME_BIN_DIR}"
            $<TARGET_FILE_DIR:perapera_anime_maker>
    )
endif()



# final_spec_v6 Phase 2 Step 1-d: Cut Cell Timesheet bridge selftest.
if (NOT TARGET perapera_cut_cell_timesheet_bridge_selftest)
    add_executable(perapera_cut_cell_timesheet_bridge_selftest
        tools/cut_cell_timesheet_bridge_selftest.cpp
        src/core/Timesheet.cpp
    )
    target_include_directories(perapera_cut_cell_timesheet_bridge_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    if (MSVC)
        target_compile_options(perapera_cut_cell_timesheet_bridge_selftest PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(perapera_cut_cell_timesheet_bridge_selftest PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

# final_spec_v6 Phase 2 Step 2-a: Project layout path selftest.
add_executable(perapera_project_layout_paths_selftest
    tools/project_layout_paths_selftest.cpp
)
target_include_directories(perapera_project_layout_paths_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
if (MSVC)
    target_compile_options(perapera_project_layout_paths_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_project_layout_paths_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# final_spec_v6 Phase 2 Step 2-b: scene.json / cut.json minimal save selftest.
add_executable(perapera_scene_cut_layout_io_selftest
    tools/scene_cut_layout_io_selftest.cpp
    src/io/SceneCutLayoutIO.cpp
    src/io/CutIO.cpp
    src/io/TimesheetIO.cpp
    src/core/Timesheet.cpp
)
target_include_directories(perapera_scene_cut_layout_io_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(perapera_scene_cut_layout_io_selftest PRIVATE nlohmann_json::nlohmann_json)
if (MSVC)
    target_compile_options(perapera_scene_cut_layout_io_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_scene_cut_layout_io_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()


# PERAPERA_PHASE2_STEP2C_BEGIN
# final_spec_v6 Phase 2 Step 2-c: cell layout minimal save.
target_sources(perapera_anime_maker PRIVATE
    src/io/CellLayoutIO.cpp
)

add_executable(perapera_cell_layout_io_selftest
    tools/cell_layout_io_selftest.cpp
    src/io/CellLayoutIO.cpp
)
target_include_directories(perapera_cell_layout_io_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(perapera_cell_layout_io_selftest PRIVATE nlohmann_json::nlohmann_json)
if (MSVC)
    target_compile_options(perapera_cell_layout_io_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_cell_layout_io_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()
# PERAPERA_PHASE2_STEP2C_END



# final_spec_v6 Phase 2 Step 2-d: layer layout minimal save selftest.
add_executable(perapera_layer_layout_io_selftest
    tools/layer_layout_io_selftest.cpp
    src/io/LayerLayoutIO.cpp
    src/core/Layer.cpp
    src/core/Stroke.cpp
)
target_include_directories(perapera_layer_layout_io_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(perapera_layer_layout_io_selftest PRIVATE nlohmann_json::nlohmann_json)
if (MSVC)
    target_compile_options(perapera_layer_layout_io_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_layer_layout_io_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# final_spec_v6 Phase 2 Step 2-e: new-layout save entry selftest.
add_executable(perapera_project_layout_save_entry_selftest
    tools/project_layout_save_entry_selftest.cpp
    src/io/ProjectLayoutSaveEntry.cpp
    src/io/SceneCutLayoutIO.cpp
    src/io/CellLayoutIO.cpp
    src/io/LayerLayoutIO.cpp
    src/io/CutIO.cpp
    src/io/TimesheetIO.cpp
    src/core/Timesheet.cpp
    src/core/Layer.cpp
    src/core/Stroke.cpp
)
target_include_directories(perapera_project_layout_save_entry_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(perapera_project_layout_save_entry_selftest PRIVATE nlohmann_json::nlohmann_json)
if (MSVC)
    target_compile_options(perapera_project_layout_save_entry_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_project_layout_save_entry_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()

# final_spec_v6 Phase 2 Step 2-g: new-layout read/inspection selftest.
add_executable(perapera_project_layout_read_entry_selftest
    tools/project_layout_read_entry_selftest.cpp
    src/io/ProjectLayoutReadEntry.cpp
    src/io/ProjectLayoutSaveEntry.cpp
    src/io/SceneCutLayoutIO.cpp
    src/io/CellLayoutIO.cpp
    src/io/LayerLayoutIO.cpp
    src/io/CutIO.cpp
    src/io/TimesheetIO.cpp
    src/core/Timesheet.cpp
    src/core/Layer.cpp
    src/core/Stroke.cpp
)
target_include_directories(perapera_project_layout_read_entry_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(perapera_project_layout_read_entry_selftest PRIVATE nlohmann_json::nlohmann_json)
if (MSVC)
    target_compile_options(perapera_project_layout_read_entry_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_project_layout_read_entry_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()


# PERAPERA_PHASE2_STEP2H_BEGIN
# final_spec_v6 Phase 2 Step 2-h: new-layout load entry.
target_sources(perapera_anime_maker PRIVATE
    src/io/ProjectLayoutLoadEntry.cpp
)

add_executable(perapera_project_layout_load_entry_selftest
    tools/project_layout_load_entry_selftest.cpp
    src/io/ProjectLayoutLoadEntry.cpp
    src/io/ProjectLayoutSaveEntry.cpp
    src/io/SceneCutLayoutIO.cpp
    src/io/CellLayoutIO.cpp
    src/io/LayerLayoutIO.cpp
    src/io/CutIO.cpp
    src/io/TimesheetIO.cpp
    src/core/Timesheet.cpp
    src/core/Layer.cpp
    src/core/Stroke.cpp
)
target_include_directories(perapera_project_layout_load_entry_selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(perapera_project_layout_load_entry_selftest PRIVATE nlohmann_json::nlohmann_json)
if (MSVC)
    target_compile_options(perapera_project_layout_load_entry_selftest PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(perapera_project_layout_load_entry_selftest PRIVATE -Wall -Wextra -Wpedantic)
endif()
# PERAPERA_PHASE2_STEP2H_END
