// このファイルの役割:
// 正式タイムシートv1を持つCut単位の保存・読み込み入口を定義する。
// Step 2.5ではProject / UI / 描画には接続せず、cutフォルダ内の cut.json と timesheet.json の約束を固定する。

#pragma once

#include "core/Cut.h"

#include <filesystem>
#include <string>

namespace perapera::CutIO {

// cutフォルダ直下に置く正式カットメタデータファイル名。
std::filesystem::path cutJsonPathForCutFolder(const std::filesystem::path& cutFolder);

// cutフォルダ直下に置く正式タイムシートファイル名。
// 実体の保存・読み込みは TimesheetIO が担当する。
std::filesystem::path timesheetJsonPathForCutFolder(const std::filesystem::path& cutFolder);

// Cutをcutフォルダへ保存する。
// cut.json にはカットのメタ情報とセル列順を保存し、timesheet.json にはタイムシート本体を保存する。
bool saveCut(
    const Cut& cut,
    const std::filesystem::path& cutFolder,
    std::string* errorMessage = nullptr);

// Cutをcutフォルダから読み込む。
// timesheet.json がない場合はfalseを返すが、cutには安全な初期値を入れる。
bool loadCut(
    const std::filesystem::path& cutFolder,
    Cut& cut,
    std::string* errorMessage = nullptr);

} // namespace perapera::CutIO
