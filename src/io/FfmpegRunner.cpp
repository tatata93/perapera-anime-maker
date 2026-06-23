// このファイルの役割:
// FFmpegの外部コマンド呼び出しを実装する。
// MP4失敗理由を追えるよう、FFmpeg実行前の検査結果とコマンドを必ずログへ保存する。

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

std::string quoteRaw(const std::string& text)
{
    return "\"" + text + "\"";
}

std::string quotePath(const std::filesystem::path& path)
{
    return quoteRaw(path.string());
}

std::string quoteExecutableIfNeeded(const std::string& executable)
{
    if (hasPathSeparator(executable) || hasWhitespace(executable)) {
        return quoteRaw(executable);
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
    return inputPattern.parent_path() / "frame_001.png";
}

std::filesystem::path logPathForOutput(const std::filesystem::path& outputPath)
{
    const std::filesystem::path parent = outputPath.parent_path().empty()
        ? std::filesystem::current_path()
        : outputPath.parent_path();
    return parent / "ffmpeg_last_run.log";
}

void appendLog(const std::filesystem::path& logPath, const std::string& text)
{
    std::error_code errorCode;
    std::filesystem::create_directories(logPath.parent_path(), errorCode);
    std::ofstream log(logPath, std::ios::binary | std::ios::app);
    if (log) {
        log << text;
    }
}

void resetLog(const std::filesystem::path& logPath)
{
    std::error_code errorCode;
    std::filesystem::create_directories(logPath.parent_path(), errorCode);
    std::ofstream log(logPath, std::ios::binary | std::ios::trunc);
    if (log) {
        log << "perapera-anime-maker FFmpeg log\n";
        log << "=================================\n\n";
    }
}

bool executableLooksLikePath(const std::string& executable)
{
    return hasPathSeparator(executable);
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

    std::ostringstream command;
    command << quoteExecutableIfNeeded(ffmpegPath)
            << " -y"
            << " -hide_banner"
            << " -framerate " << fps
            << " -start_number 1"
            << " -i " << quotePath(inputPattern)
            << " -vf " << quoteRaw("format=yuv420p")
            << " -c:v libx264"
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
    const std::filesystem::path logPath = logPathForOutput(outputPath);
    resetLog(logPath);

    appendLog(logPath, "ffmpegPath: " + ffmpegPath + "\n");
    appendLog(logPath, "inputPattern: " + inputPattern.string() + "\n");
    appendLog(logPath, "outputPath: " + outputPath.string() + "\n");
    appendLog(logPath, "fps: " + std::to_string(fps) + "\n\n");

    if (ffmpegPath.empty()) {
        const std::string message = "FFmpeg path is empty. log: " + logPath.string();
        appendLog(logPath, "ERROR: FFmpeg path is empty.\n");
        setError(errorMessage, message);
        return false;
    }

    std::error_code errorCode;
    if (executableLooksLikePath(ffmpegPath) && !std::filesystem::exists(std::filesystem::path(ffmpegPath), errorCode)) {
        const std::string message = "FFmpeg executable not found: " + ffmpegPath + " log: " + logPath.string();
        appendLog(logPath, "ERROR: FFmpeg executable not found.\n");
        setError(errorMessage, message);
        return false;
    }

    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path(), errorCode);
        if (errorCode) {
            const std::string message = "failed to create MP4 folder: " + errorCode.message() + " log: " + logPath.string();
            appendLog(logPath, "ERROR: " + message + "\n");
            setError(errorMessage, message);
            return false;
        }
    }

    const std::filesystem::path firstFrame = firstFramePathFromPattern(inputPattern);
    if (!std::filesystem::exists(firstFrame, errorCode)) {
        const std::string message = "first PNG frame not found: " + firstFrame.string() + " log: " + logPath.string();
        appendLog(logPath, "ERROR: first PNG frame not found.\n");
        appendLog(logPath, "Expected: " + firstFrame.string() + "\n");
        setError(errorMessage, message);
        return false;
    }

    const std::string command = buildPngSequenceCommand(ffmpegPath, inputPattern, fps, outputPath);
    appendLog(logPath, "Command:\n" + command + "\n\nOutput:\n");

    const int result = std::system(command.c_str());
    if (result != 0) {
        const std::string message = "ffmpeg command failed. exit=" + std::to_string(result) + " log: " + logPath.string();
        appendLog(logPath, "\nERROR: " + message + "\n");
        setError(errorMessage, message);
        return false;
    }

    if (!std::filesystem::exists(outputPath, errorCode)) {
        const std::string message = "ffmpeg finished but MP4 was not found. log: " + logPath.string();
        appendLog(logPath, "\nERROR: MP4 file was not created.\n");
        setError(errorMessage, message);
        return false;
    }

    appendLog(logPath, "\nSUCCESS: MP4 created.\n");
    return true;
}

} // namespace perapera
