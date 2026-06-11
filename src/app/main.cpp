// src/app/main.cpp
//
// このファイルは「ぺらぺらアニメ作り機」の最小エントリーポイントです。
//
// Step 0では、まだGUI、作画、カメラ、3Dガイドは実装しません。
// まず「C++20 + CMakeでビルドできる土台」を確認するための最小プログラムです。

#include <iostream>

#ifdef _WIN32
    // Windowsのコンソールで日本語をUTF-8表示しやすくするために使う。
    // Windows以外ではこのヘッダーは存在しないので、_WIN32のときだけ読み込む。
    #include <windows.h>
#endif

// main関数はC++プログラムの開始地点です。
// 今は、プロジェクトが正しくビルド・実行できることを確認するために文字を表示します。
int main()
{
#ifdef _WIN32
    // Windowsコンソールの出力文字コードをUTF-8にする。
    // これにより、日本語の「ぺらぺらアニメ作り機」が文字化けしにくくなる。
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::cout << "ぺらぺらアニメ作り機 Step 0: C++20 project is working." << std::endl;
    std::cout << "Internal project name: PeraperaAnimeMaker" << std::endl;
    std::cout << "Internal executable name: perapera_anime_maker" << std::endl;
    std::cout << "Next step: add SDL3 and Dear ImGui only after approval." << std::endl;

    return 0;
}