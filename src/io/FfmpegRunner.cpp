// このファイルの役割:
// FFmpegの外部コマンド呼び出しを実装する。
// WindowsではCreateProcessで直接起動し、cmd/batchの%展開や引用符問題を避ける。

#include "io/FfmpegRunner.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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
        log << "perapera-anime-maker FFmpeg log v10\n";
        log << "==================================\n\n";
    }
}

bool executableLooksLikePath(const std::string& executable)
{
    return hasPathSeparator(executable);
}

#ifdef _WIN32
std::wstring toWidePath(const std::filesystem::path& path)
{
    return path.wstring();
}

std::wstring toWideUtf8ish(const std::string& text)
{
    return std::filesystem::path(text).wstring();
}

std::wstring quoteArg(const std::wstring& value)
{
    std::wstring result = L"\"";
    std::size_t backslashCount = 0;
    for (wchar_t ch : value) {
        if (ch == L'\\') {
            ++backslashCount;
            result.push_back(ch);
        } else if (ch == L'\"') {
            result.append(backslashCount, L'\\');
            result.push_back(L'\\');
            result.push_back(ch);
            backslashCount = 0;
        } else {
            backslashCount = 0;
            result.push_back(ch);
        }
    }
    result.append(backslashCount, L'\\');
    result.push_back(L'\"');
    return result;
}

std::wstring buildWindowsCommandLine(const std::string& ffmpegPath,
                                     const std::filesystem::path& inputPattern,
                                     int fps,
                                     const std::filesystem::path& outputPath)
{
    if (fps <= 0) {
        fps = 24;
    }

    std::wstring command;
    command += quoteArg(toWideUtf8ish(ffmpegPath));
    command += L" -y -hide_banner -f image2 -framerate ";
    command += std::to_wstring(fps);
    command += L" -start_number 1 -i ";
    command += quoteArg(toWidePath(inputPattern));
    command += L" -vf ";
    command += quoteArg(L"format=yuv420p");
    command += L" -c:v libx264 -movflags +faststart ";
    command += quoteArg(toWidePath(outputPath));
    return command;
}

bool runWindowsProcess(const std::wstring& commandLine,
                       const std::filesystem::path& logPath,
                       int* exitCode,
                       std::string* errorMessage)
{
    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.bInheritHandle = TRUE;

    HANDLE logHandle = CreateFileW(toWidePath(logPath).c_str(),
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &securityAttributes,
                                   OPEN_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
    if (logHandle == INVALID_HANDLE_VALUE) {
        setError(errorMessage, "failed to open FFmpeg log for process output");
        return false;
    }

    SetFilePointer(logHandle, 0, nullptr, FILE_END);

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startupInfo.hStdOutput = logHandle;
    startupInfo.hStdError = logHandle;

    PROCESS_INFORMATION processInfo{};
    std::wstring mutableCommand = commandLine;
    const BOOL started = CreateProcessW(nullptr,
                                        mutableCommand.data(),
                                        nullptr,
                                        nullptr,
                                        TRUE,
                                        CREATE_NO_WINDOW,
                                        nullptr,
                                        nullptr,
                                        &startupInfo,
                                        &processInfo);
    CloseHandle(logHandle);

    if (!started) {
        setError(errorMessage, "CreateProcessW failed. error=" + std::to_string(GetLastError()));
        return false;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD processExitCode = 1;
    GetExitCodeProcess(processInfo.hProcess, &processExitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    if (exitCode != nullptr) {
        *exitCode = static_cast<int>(processExitCode);
    }
    return processExitCode == 0;
}
#endif

} // namespace

std::string FfmpegRunner::buildPngSequenceCommand(const std::string& ffmpegPath,
                                                  const std::filesystem::path& inputPattern,
                                                  int fps,
                                                  const std::filesystem::path& outputPath)
{
    if (fps <= 0) {
        fps = 24;
    }

    std::ostringstream command;
    command << quoteExecutableIfNeeded(ffmpegPath)
            << " -y -hide_banner -f image2 -framerate " << fps
            << " -start_number 1"
            << " -i " << quotePath(inputPattern)
            << " -vf " << quoteRaw("format=yuv420p")
            << " -c:v libx264 -movflags +faststart "
            << quotePath(outputPath);
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
        appendLog(logPath, "ERROR: first PNG frame not found.\nExpected: " + firstFrame.string() + "\n");
        setError(errorMessage, message);
        return false;
    }

    const std::string command = buildPngSequenceCommand(ffmpegPath, inputPattern, fps, outputPath);
    appendLog(logPath, "Command preview:\n" + command + "\n\nOutput:\n");

#ifdef _WIN32
    int exitCode = 1;
    const std::wstring commandLine = buildWindowsCommandLine(ffmpegPath, inputPattern, fps, outputPath);
    if (!runWindowsProcess(commandLine, logPath, &exitCode, errorMessage)) {
        const std::string message = "ffmpeg command failed. exit=" + std::to_string(exitCode) + " log: " + logPath.string();
        appendLog(logPath, "\nERROR: " + message + "\n");
        setError(errorMessage, message);
        return false;
    }
#else
    const std::string runCommand = command + " >> " + quotePath(logPath) + " 2>&1";
    const int result = std::system(runCommand.c_str());
    if (result != 0) {
        const std::string message = "ffmpeg command failed. exit=" + std::to_string(result) + " log: " + logPath.string();
        appendLog(logPath, "\nERROR: " + message + "\n");
        setError(errorMessage, message);
        return false;
    }
#endif

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
