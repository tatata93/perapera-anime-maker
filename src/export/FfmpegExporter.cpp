// src/export/FfmpegExporter.cpp

#include "export/FfmpegExporter.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <system_error>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

namespace perapera
{
    namespace
    {
        std::string quoteForDisplay(const std::string& value)
        {
            if (value.find_first_of(" \t\"") == std::string::npos)
            {
                return value;
            }

            std::string quoted = "\"";

            for (char character : value)
            {
                if (character == '"')
                {
                    quoted += '\\';
                }

                quoted += character;
            }

            quoted += '"';
            return quoted;
        }

#if !defined(_WIN32)
        std::string quotePosixShellArgument(const std::string& value)
        {
            std::string quoted = "'";

            for (char character : value)
            {
                if (character == '\'')
                {
                    quoted += "'\\''";
                }
                else
                {
                    quoted += character;
                }
            }

            quoted += '\'';
            return quoted;
        }
#endif

#if defined(_WIN32)
        std::wstring quoteWindowsArgument(const std::wstring& value)
        {
            if (value.empty())
            {
                return L"\"\"";
            }

            if (value.find_first_of(L" \t\n\v\"") == std::wstring::npos)
            {
                return value;
            }

            std::wstring quoted = L"\"";
            unsigned int backslashCount = 0;

            for (wchar_t character : value)
            {
                if (character == L'\\')
                {
                    ++backslashCount;
                    continue;
                }

                if (character == L'"')
                {
                    quoted.append(backslashCount * 2 + 1, L'\\');
                    quoted += L'"';
                    backslashCount = 0;
                    continue;
                }

                quoted.append(backslashCount, L'\\');
                backslashCount = 0;
                quoted += character;
            }

            quoted.append(backslashCount * 2, L'\\');
            quoted += L'"';
            return quoted;
        }

        std::wstring utf8ToWide(const std::string& value)
        {
            if (value.empty())
            {
                return std::wstring();
            }

            const int requiredSize = MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                value.data(),
                static_cast<int>(value.size()),
                nullptr,
                0
            );

            if (requiredSize <= 0)
            {
                return std::wstring(value.begin(), value.end());
            }

            std::wstring result(static_cast<std::size_t>(requiredSize), L'\0');
            MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                value.data(),
                static_cast<int>(value.size()),
                result.data(),
                requiredSize
            );
            return result;
        }

        FfmpegExportResult runWindowsProcess(
            const std::filesystem::path& executablePath,
            const std::vector<std::string>& arguments,
            const std::string& displayCommand
        )
        {
            FfmpegExportResult result;
            result.command = displayCommand;

            SECURITY_ATTRIBUTES securityAttributes{};
            securityAttributes.nLength = sizeof(securityAttributes);
            securityAttributes.bInheritHandle = TRUE;

            HANDLE outputRead = nullptr;
            HANDLE outputWrite = nullptr;

            if (!CreatePipe(
                &outputRead,
                &outputWrite,
                &securityAttributes,
                0
            ))
            {
                result.errorMessage =
                    "FFmpegログ取得用パイプを作成できませんでした。";
                return result;
            }

            SetHandleInformation(outputRead, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOW startupInfo{};
            startupInfo.cb = sizeof(startupInfo);
            startupInfo.dwFlags = STARTF_USESTDHANDLES;
            startupInfo.hStdOutput = outputWrite;
            startupInfo.hStdError = outputWrite;
            startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

            PROCESS_INFORMATION processInformation{};

            const std::wstring executableWide = executablePath.wstring();
            std::wstring commandLine = quoteWindowsArgument(executableWide);

            for (const std::string& argument : arguments)
            {
                commandLine += L' ';
                commandLine += quoteWindowsArgument(utf8ToWide(argument));
            }

            std::vector<wchar_t> mutableCommandLine(
                commandLine.begin(),
                commandLine.end()
            );
            mutableCommandLine.push_back(L'\0');

            const BOOL created = CreateProcessW(
                nullptr,
                mutableCommandLine.data(),
                nullptr,
                nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr,
                nullptr,
                &startupInfo,
                &processInformation
            );

            const DWORD createError = created ? ERROR_SUCCESS : GetLastError();
            CloseHandle(outputWrite);

            if (!created)
            {
                CloseHandle(outputRead);
                result.errorMessage =
                    "FFmpegを起動できませんでした。Windowsエラー: "
                    + std::to_string(createError);
                return result;
            }

            std::array<char, 4096> buffer{};
            DWORD bytesRead = 0;

            while (ReadFile(
                outputRead,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &bytesRead,
                nullptr
            ) && bytesRead > 0)
            {
                result.log.append(buffer.data(), bytesRead);
            }

            WaitForSingleObject(processInformation.hProcess, INFINITE);

            DWORD exitCode = 1;
            GetExitCodeProcess(processInformation.hProcess, &exitCode);

            CloseHandle(outputRead);
            CloseHandle(processInformation.hThread);
            CloseHandle(processInformation.hProcess);

            result.exitCode = static_cast<int>(exitCode);
            result.succeeded = (exitCode == 0);

            if (!result.succeeded)
            {
                result.errorMessage =
                    "FFmpegがエラー終了しました。終了コード: "
                    + std::to_string(result.exitCode);
            }

            return result;
        }
#else
        FfmpegExportResult runPosixProcess(
            const std::filesystem::path& executablePath,
            const std::vector<std::string>& arguments,
            const std::string& displayCommand
        )
        {
            FfmpegExportResult result;
            result.command = displayCommand;

            std::ostringstream command;
            command << quotePosixShellArgument(executablePath.string());

            for (const std::string& argument : arguments)
            {
                command << ' ' << quotePosixShellArgument(argument);
            }

            command << " 2>&1";

            FILE* pipe = popen(command.str().c_str(), "r");

            if (pipe == nullptr)
            {
                result.errorMessage = "FFmpegを起動できませんでした。";
                return result;
            }

            std::array<char, 4096> buffer{};

            while (std::fgets(buffer.data(), buffer.size(), pipe) != nullptr)
            {
                result.log += buffer.data();
            }

            result.exitCode = pclose(pipe);
            result.succeeded = (result.exitCode == 0);

            if (!result.succeeded)
            {
                result.errorMessage =
                    "FFmpegがエラー終了しました。終了コード: "
                    + std::to_string(result.exitCode);
            }

            return result;
        }
#endif
    }

    std::vector<std::string> FfmpegExporter::buildArguments(
        const std::filesystem::path& inputPattern,
        const std::filesystem::path& outputPath,
        const FfmpegSettings& settings
    )
    {
        const int safeFps = std::clamp(settings.framesPerSecond, 1, 240);
        const int safeCrf = std::clamp(settings.constantRateFactor, 0, 51);
        const std::string safePreset = settings.preset.empty()
            ? "medium"
            : settings.preset;

        return {
            "-hide_banner",
            settings.overwriteOutput ? "-y" : "-n",
            "-framerate",
            std::to_string(safeFps),
            "-start_number",
            "1",
            "-i",
            inputPattern.string(),
            "-c:v",
            "libx264",
            "-preset",
            safePreset,
            "-crf",
            std::to_string(safeCrf),
            "-vf",
            "pad=ceil(iw/2)*2:ceil(ih/2)*2",
            "-pix_fmt",
            "yuv420p",
            "-movflags",
            "+faststart",
            outputPath.string()
        };
    }

    std::string FfmpegExporter::formatCommandForDisplay(
        const std::filesystem::path& executablePath,
        const std::vector<std::string>& arguments
    )
    {
        std::ostringstream command;
        command << quoteForDisplay(executablePath.string());

        for (const std::string& argument : arguments)
        {
            command << ' ' << quoteForDisplay(argument);
        }

        return command.str();
    }

    FfmpegExportResult FfmpegExporter::exportPngSequenceToMp4(
        const std::filesystem::path& inputPattern,
        const std::filesystem::path& outputPath,
        const FfmpegSettings& settings
    )
    {
        FfmpegExportResult result;

        if (settings.executablePath.empty())
        {
            result.errorMessage = "FFmpeg実行ファイルのパスが空です。";
            return result;
        }

        const std::filesystem::path firstInputFrame =
            inputPattern.parent_path() / "frame_0001.png";

        if (!std::filesystem::exists(firstInputFrame))
        {
            result.errorMessage =
                "入力PNGが見つかりません: "
                + firstInputFrame.string();
            return result;
        }

        std::error_code directoryError;

        if (outputPath.has_parent_path())
        {
            std::filesystem::create_directories(
                outputPath.parent_path(),
                directoryError
            );
        }

        if (directoryError)
        {
            result.errorMessage =
                "動画出力フォルダーを作成できません: "
                + directoryError.message();
            return result;
        }

        const std::vector<std::string> arguments =
            buildArguments(inputPattern, outputPath, settings);
        const std::string displayCommand =
            formatCommandForDisplay(settings.executablePath, arguments);

#if defined(_WIN32)
        return runWindowsProcess(
            settings.executablePath,
            arguments,
            displayCommand
        );
#else
        return runPosixProcess(
            settings.executablePath,
            arguments,
            displayCommand
        );
#endif
    }
}
