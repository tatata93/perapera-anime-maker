cmake_minimum_required(VERSION 3.22)

# ぺらぺらアニメ作り機
# C++で手描き2Dアニメ制作ソフトを作るためのプロジェクト。
#
# 注意:
# アプリの正式名称は日本語の「ぺらぺらアニメ作り機」だが、
# CMakeプロジェクト名や実行ファイル名は、環境差による文字化けや
# パス問題を避けるため、ASCII名の PeraperaAnimeMaker を使う。

project(PeraperaAnimeMaker
    VERSION 0.0.1
    LANGUAGES CXX
)

# C++20を使う。
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 実行ファイルの出力先を build/bin にまとめる。
# Visual Studio系のジェネレーターでも Ninja でも同じ場所に出しやすくするため。
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# 実行ファイルを作る。
# 出力名は日本語ではなく perapera_anime_maker にする。
add_executable(perapera_anime_maker
    src/app/main.cpp
)

# コンパイラごとの警告設定。
# 初期段階から警告を見やすくして、危ないコードを早めに見つける。
if(MSVC)
    # /W4 は警告を多めに出す設定。
    # /utf-8 はソースコード内の日本語文字列をUTF-8として扱うための設定。
    target_compile_options(perapera_anime_maker PRIVATE /W4 /utf-8)
else()
    target_compile_options(perapera_anime_maker PRIVATE -Wall -Wextra -Wpedantic)
endif()