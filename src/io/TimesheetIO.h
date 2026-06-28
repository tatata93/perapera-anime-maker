// このファイルの役割:
// 正式タイムシートv1を JSON ファイルとして保存・読み込みする入口を定義する。
// Project / UI / 描画には接続せず、core の Timesheet だけを扱う。

#pragma once

#include "core/Timesheet.h"

#include <filesystem>
#include <string>

namespace perapera::TimesheetIO {

// cut フォルダ直下に置く正式タイムシートファイル名。
// Step 2ではProject/Cutへ接続しないが、保存場所の約束だけ先に固定する。
std::filesystem::path timesheetPathForCutFolder(const std::filesystem::path& cutFolder);

// path に正式タイムシートJSONを書き出す。
// 同じ内容ならファイルを更新しない。
bool saveTimesheet(
    const Timesheet& timesheet,
    const std::filesystem::path& path,
    std::string* errorMessage = nullptr);

// path から正式タイムシートJSONを読み込む。
// 失敗時は false を返し、timesheet には安全な初期値を入れる。
bool loadTimesheet(
    const std::filesystem::path& path,
    Timesheet& timesheet,
    std::string* errorMessage = nullptr);

} // namespace perapera::TimesheetIO
