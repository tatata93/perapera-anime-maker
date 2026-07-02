// このファイルの役割:
// 正式タイムシートv1の中核データモデルを定義する。
// ここではUI、保存形式、描画処理へ依存しない。
// タイムラインTと作画Fを明確に分離するための最小モデルである。

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace perapera {

// タイムシート1マスの意味。
// timelineFrame は紙タイムシートに合わせて 1 始まりで扱う。
enum class TimesheetEntryType {
    Hold,       // 前の状態を保持する。前に表示中の絵がなければ非表示のまま。
    Drawing,    // 通常の作画F指定。
    Null,       // 空セル。以後のHoldも非表示を保持する。
    Key,        // 原画としての作画F指定。
    Inbetween,  // 中割としての作画F指定。
};

const char* timesheetEntryTypeToString(TimesheetEntryType type) noexcept;
TimesheetEntryType timesheetEntryTypeFromString(std::string_view value) noexcept;
const char* timesheetEntryTypeJapaneseLabel(TimesheetEntryType type) noexcept;
bool timesheetEntryTypeUsesDrawingNumber(TimesheetEntryType type) noexcept;

struct TimesheetEntry {
    int timelineFrame = 1;                         // T。1始まり。
    TimesheetEntryType type = TimesheetEntryType::Hold;
    int drawingFrameNumber = 0;                    // 作画F番号。UI表示は1始まり。0は未指定。
    std::string note;                              // そのマスだけの制作メモ。
};

struct TimesheetCellTrack {
    std::string cellId;                            // 既存Cellのidへ接続するキー。
    std::string displayName;                       // UI表示名。空ならCell側の名前を使う想定。
    int defaultExposure = 1;                       // 将来の自動配置用。MVPでは解決処理に使わない。
    std::vector<TimesheetEntry> entries;           // 記入されたマスだけを保持する疎データ。
};

struct TimesheetFrameNote {
    int timelineFrame = 1;                         // T。1始まり。
    std::string dialogue;                          // セリフ。
    std::string cameraInstruction;                 // カメラ指示。
    std::string shootingInstruction;               // 撮影指示。
    std::string materialMemo;                      // 背景、BOOK、素材メモ。
};

struct Timesheet {
    int totalFrames = 144;                         // カット尺。1以上に正規化する。
    int defaultExposure = 1;                       // 新規入力時の既定打ち。
    std::vector<TimesheetCellTrack> tracks;        // 横方向のセル列。
    std::vector<TimesheetFrameNote> frameNotes;    // 追加欄。
};

int normalizeTimesheetFrameCount(int totalFrames) noexcept;
int clampTimesheetFrame(const Timesheet& timesheet, int timelineFrame) noexcept;

TimesheetCellTrack* findTimesheetTrack(Timesheet& timesheet, std::string_view cellId) noexcept;
const TimesheetCellTrack* findTimesheetTrack(const Timesheet& timesheet, std::string_view cellId) noexcept;
TimesheetCellTrack& ensureTimesheetTrack(Timesheet& timesheet, std::string_view cellId, std::string_view displayName = {});

void sortTimesheetTrackEntries(TimesheetCellTrack& track);
void normalizeTimesheet(Timesheet& timesheet);

} // namespace perapera
