// このファイルの役割:
// FFmpegの外部コマンド呼び出しを実装する。
// 失敗時に原因を追えるよう、最後に実行したFFmpegのログを exports/mp4/ffmpeg_last_run.log に保存する。

#include "io/FfmpegRunner.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace perapera {
namespace {

bool hasPathSeparator(const std::string& text)
{
    return text.find('/') != std::string::npos || text.find('\\') != std::string::npos;
}

bool hasWhitespace(const std::string& text)
{
    return text.find(' ') != std::string::npos || text.find('\t') != std::string::npos;
}

std::string quotePath(const std::filesystem::path& path)
{
    return "\"" + path.string() + "\"";
}

std::string quoteExecutableIfNeeded(const std::string& executable)
{
    // ffmpeg のようにPATH検索させる名前は引用符なしの方がWindowsのcmdで安定する。
    // C:/Program Files/.../ffmpeg.exe のようなパスだけ引用する。
    if (hasPathSeparator(executable) || hasWhitespace(executable)) {
        return "\"" + executable + "\"";
    }
    return executable;
}

void setError(std::string* errorMessage, const std::string& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

std::filesystem::path firstFramePathFromPattern(const std::filesystem::path& inputPattern)
{
    // 現在のPngExporterは frame_001.png から出力する。
    // 入力パターンの親フォルダを使って、最初のPNGが実在するか確認する。
    return inputPattern.parent_path() / "frame_001.png";
}

std::filesystem::path logPathForOutput(const std::filesystem::path& outputPath)
{
    const std::filesystem::path parent = outputPath.parent_path().empty()
        ? std::filesystem::current_path()
        : outputPath.parent_path();
    return parent / "ffmpeg_last_run.log";
}

void writeCommandLog(const std::filesystem::path& logPath, const std::string& command)
{
    std::error_code errorCode;
    std::filesystem::create_directories(logPath.parent_path(), errorCode);
    std::ofstream log(logPath, std::ios::binary);
    if (log) {
        log << "Command:\n" << command << "\n\nOutput:\n";
    }
}

} // namespace

std::string FfmpegRunner::buildPngSequenceCommand(const std::string& ffmpegPath,
                                                  const std::filesystem::path& inputPattern,
                                                  int fps,
                                                  const std::filesystem::path& outputPath)
{
    if (fps <= 0) {
        fps = 24;
    }

    const std::filesystem::path logPath = logPathForOutput(outputPath);

    // PngExporterは frame_001.png, frame_002.png ... を作る。
    // FFmpegのimage2入力は初期値だとframe_000.pngを探すことがあるため、-start_number 1 を明示する。
    std::ostringstream command;
    command << quoteExecutableIfNeeded(ffmpegPath)
            << " -y"
            << " -framerate " << fps
            << " -start_number 1"
            << " -i " << quotePath(inputPattern)
            << " -c:v libx264"
            << " -pix_fmt yuv420p"
            << " -movflags +faststart "
            << quotePath(outputPath)
            << " >> " << quotePath(logPath)
            << " 2>&1";
    return command.str();
}

bool FfmpegRunner::pngSequenceToMp4(const std::string& ffmpegPath,
                                    const std::filesystem::path& inputPattern,
                                    int fps,
                                    const std::filesystem::path& outputPath,
                                    std::string* errorMessage)
{
    if (ffmpegPath.empty()) {
        setError(errorMessage, "FFmpeg path is empty");
        return false;
    }

    std::error_code errorCode;
    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path(), errorCode);
        if (errorCode) {
            setError(errorMessage, errorCode.message());
            return false;
        }
    }

    const std::filesystem::path firstFrame = firstFramePathFromPattern(inputPattern);
    if (!std::filesystem::exists(firstFrame, errorCode)) {
        setError(errorMessage, "first PNG frame not found: " + firstFrame.string());
        return false;
    }

    const std::filesystem::path logPath = logPathForOutput(outputPath);
    const std::string command = buildPngSequenceCommand(ffmpegPath, inputPattern, fps, outputPath);
    writeCommandLog(logPath, command);

    const int result = std::system(command.c_str());
    if (result != 0) {
        setError(errorMessage, "ffmpeg command failed. log: " + logPath.string() + " command: " + command);
        return false;
    }

    if (!std::filesystem::exists(outputPath, errorCode)) {
        setError(errorMessage, "ffmpeg finished but MP4 was not found. log: " + logPath.string());
        return false;
    }

    return true;
}

} // namespace perapera
