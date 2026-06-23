#pragma once

// このファイルの役割:
// Project を仕様書のフォルダ構成に保存・読み込みする入口を定義する。
// composite.png は保存対象にしない。点列JSONを正とする。

#include <filesystem>
#include <string>

#include "core/Project.h"

namespace perapera {

class ProjectIO {
public:
    // folderPath 直下に project.json と cells/ 以下を書き出す。
    // 失敗時は false を返し、errorMessage があれば理由を書く。
    static bool save(const Project& project,
                     const std::filesystem::path& folderPath,
                     std::string* errorMessage = nullptr);

    // folderPath 直下の project.json と cells/ 以下を読み込む。
    // 失敗時は false を返し、project には安全な初期値を入れる。
    static bool load(const std::filesystem::path& folderPath,
                     Project& project,
                     std::string* errorMessage = nullptr);
};

} // namespace perapera
