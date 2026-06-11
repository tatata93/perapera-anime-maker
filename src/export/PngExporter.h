// src/export/PngExporter.h
//
// PngExporterは、現在の作画データをPNG画像として保存する処理を担当します。
//
// Phase 3Cでは、
// - 表示中レイヤーだけを書き出す
// - レイヤー不透明度を反映する
// - 撮影フレーム範囲をPNGとして保存する
// ところまでを実装します。
//
// 注意:
// ここではUI部品を描きません。
// UIはDrawingCanvasPanel、PNG変換はPngExporter、というように役割を分けます。

#pragma once

#include "drawing/DrawingLayer.h"
#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include <filesystem>
#include <string>
#include <vector>

namespace perapera
{
    struct PngExportOptions
    {
        // true の場合、背景を透明にして保存する。
        // false の場合、確認しやすい濃い背景を入れて保存する。
        bool transparentBackground = false;
    };

    class PngExporter
    {
    public:
        // WorkCanvas中央に置かれている撮影フレーム範囲をPNG保存する。
        //
        // outputPath:
        // 保存先PNGパス。
        //
        // layers:
        // 作画レイヤー一覧。visible == true のレイヤーだけ書き出す。
        //
        // errorMessage:
        // 失敗理由を呼び出し元へ返すための文字列。
        static bool exportCenteredRenderFrame(
            const std::filesystem::path& outputPath,
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat,
            const std::vector<DrawingLayer>& layers,
            const PngExportOptions& options,
            std::string& errorMessage
        );
    };
}