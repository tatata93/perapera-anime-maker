// src/export/PngExporter.h
//
// PngExporterは、作画データをPNG画像として保存する処理を担当します。
//
// UIはDrawingCanvasPanel、PNG変換はPngExporter、撮影範囲はShotCamera2D、
// というように役割を分けます。

#pragma once

#include "drawing/DrawingLayer.h"
#include "drawing/WorkCanvas.h"
#include "project/RenderFormat.h"

#include <filesystem>
#include <string>
#include <vector>

namespace perapera
{
    class ShotCamera2D;

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
        // 古い呼び出し元との互換用に残しておく。
        static bool exportCenteredRenderFrame(
            const std::filesystem::path& outputPath,
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat,
            const std::vector<DrawingLayer>& layers,
            const PngExportOptions& options,
            std::string& errorMessage
        );

        // ShotCamera2Dが指す範囲を、RenderFormatのサイズへPNG保存する。
        // パンとズームはこの関数の中で反映される。
        static bool exportShotCameraFrame(
            const std::filesystem::path& outputPath,
            const WorkCanvas& workCanvas,
            const RenderFormat& renderFormat,
            const ShotCamera2D& shotCamera,
            const std::vector<DrawingLayer>& layers,
            const PngExportOptions& options,
            std::string& errorMessage
        );
    };
}
