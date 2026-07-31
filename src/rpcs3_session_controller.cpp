#include "rpcs3_session_controller.h"

#include "app_logger.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

#include <TlHelp32.h>

#include <set>
#include <vector>

void Rpcs3SessionController::setEmulatorPath(const QString& path)
{
    emulatorPath_ = path.trimmed();
}

void Rpcs3SessionController::setGamePath(const QString& path)
{
    gamePath_ = path.trimmed();
}

QString Rpcs3SessionController::emulatorPath() const
{
    return emulatorPath_;
}

QString Rpcs3SessionController::gamePath() const
{
    return gamePath_;
}

std::optional<DWORD> Rpcs3SessionController::findProcessIdByExeName(const std::wstring& exeName)
{
    const QString targetExe = QString::fromStdWString(exeName);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return std::nullopt;
    }

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (!Process32FirstW(snapshot, &pe))
    {
        CloseHandle(snapshot);
        return std::nullopt;
    }

    do
    {
        const QString runningExe = QString::fromWCharArray(pe.szExeFile);
        if (runningExe.compare(targetExe, Qt::CaseInsensitive) == 0)
        {
            const DWORD pid = pe.th32ProcessID;
            CloseHandle(snapshot);
            return pid;
        }
    } while (Process32NextW(snapshot, &pe));

    CloseHandle(snapshot);
    return std::nullopt;
}

std::vector<DWORD> Rpcs3SessionController::findProcessIdsByExeName(const std::wstring& exeName)
{
    std::vector<DWORD> pids;
    const QString targetExe = QString::fromStdWString(exeName);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        return pids;
    }

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (!Process32FirstW(snapshot, &pe))
    {
        CloseHandle(snapshot);
        return pids;
    }

    do
    {
        const QString runningExe = QString::fromWCharArray(pe.szExeFile);
        if (runningExe.compare(targetExe, Qt::CaseInsensitive) == 0)
        {
            pids.push_back(pe.th32ProcessID);
        }
    } while (Process32NextW(snapshot, &pe));

    CloseHandle(snapshot);
    return pids;
}

std::optional<DWORD> Rpcs3SessionController::findRunningPid() const
{
    const QString configuredExeName = QFileInfo(emulatorPath_).fileName().trimmed();
    if (!configuredExeName.isEmpty())
    {
        const auto pid = findProcessIdByExeName(configuredExeName.toStdWString());
        if (pid.has_value())
        {
            return pid;
        }
    }

    static const std::wstring kFallbackExe = L"rpcs3.exe";
    return findProcessIdByExeName(kFallbackExe);
}

bool Rpcs3SessionController::setError(QString* outError, const QString& message)
{
    if (outError != nullptr)
    {
        *outError = message;
    }
    return false;
}

bool Rpcs3SessionController::validateEmulatorPath(QString* error) const
{
    if (emulatorPath_.isEmpty() || !QFileInfo::exists(emulatorPath_))
    {
        AppLogger::error(QStringLiteral("Invalid emulator path: '%1'").arg(emulatorPath_));
        return setError(error, QStringLiteral("Configure a valid RPCS3 executable path first."));
    }
    return true;
}

bool Rpcs3SessionController::validateGamePath(QString* error) const
{
    if (gamePath_.isEmpty() || !QFileInfo::exists(gamePath_))
    {
        AppLogger::error(QStringLiteral("Invalid game path: '%1'").arg(gamePath_));
        return setError(error, QStringLiteral("Configure a valid game boot target path first."));
    }
    return true;
}

bool Rpcs3SessionController::startEmulator(QString* error) const
{
    if (!validateEmulatorPath(error))
    {
        return false;
    }

    if (!ensureNoRunningRpcs3(error))
    {
        return false;
    }

    if (!launchEmulatorDetached(error))
    {
        return false;
    }

    if (!waitForRpcs3Start(5000))
    {
        AppLogger::error(QStringLiteral("Start RPCS3 failed: process did not remain running after launch."));
        return setError(error, QStringLiteral("RPCS3 did not stay running after launch. Check logs for details."));
    }

    return true;
}

bool Rpcs3SessionController::resetEmulator(int restartDelayMs, QString* error) const
{
    if (!validateEmulatorPath(error))
    {
        return false;
    }

    if (!ensureNoRunningRpcs3(error))
    {
        return false;
    }

    if (restartDelayMs < 0)
    {
        restartDelayMs = 0;
    }

    ::Sleep(static_cast<DWORD>(restartDelayMs));
    if (!launchEmulatorDetached(error))
    {
        return false;
    }

    if (!waitForRpcs3Start(5000))
    {
        AppLogger::error(QStringLiteral("Reset RPCS3 failed: process did not remain running after launch."));
        return setError(error, QStringLiteral("RPCS3 did not stay running after reset. Check logs for details."));
    }

    return true;
}

bool Rpcs3SessionController::stopEmulatorSession(QString* error) const
{
    if (!ensureNoRunningRpcs3(error))
    {
        return setError(error, QStringLiteral("Failed to terminate the active RPCS3 session."));
    }

    return true;
}

bool Rpcs3SessionController::startGame(QString* error) const
{
    if (!validateEmulatorPath(error) || !validateGamePath(error))
    {
        return false;
    }

    if (!ensureNoRunningRpcs3(error))
    {
        return false;
    }

    if (!launchGameDetached(error))
    {
        return false;
    }

    if (!waitForRpcs3Start(5000))
    {
        AppLogger::error(QStringLiteral("Start Game failed: RPCS3 process not detected after launch."));
        return setError(error, QStringLiteral("Start Game launched but RPCS3 was not running afterward. Check logs for details."));
    }

    return true;
}

bool Rpcs3SessionController::killProcessTree(DWORD pid)
{
    QProcess killer;
    killer.start(QStringLiteral("taskkill"),
                 {QStringLiteral("/PID"), QString::number(pid), QStringLiteral("/T"), QStringLiteral("/F")});

    if (!killer.waitForFinished(5000))
    {
        return false;
    }

    return killer.exitStatus() == QProcess::NormalExit && killer.exitCode() == 0;
}

bool Rpcs3SessionController::killProcessImage(const QString& imageName)
{
    if (imageName.trimmed().isEmpty())
    {
        return true;
    }

    QProcess killer;
    AppLogger::info(QStringLiteral("Executing process cleanup: taskkill /IM %1 /T /F").arg(imageName));
    killer.start(QStringLiteral("taskkill"),
                 {QStringLiteral("/IM"), imageName, QStringLiteral("/T"), QStringLiteral("/F")});

    if (!killer.waitForFinished(8000))
    {
        AppLogger::error(QStringLiteral("taskkill timeout for image %1").arg(imageName));
        return false;
    }

    if (killer.exitStatus() != QProcess::NormalExit)
    {
        AppLogger::error(QStringLiteral("taskkill abnormal exit for image %1").arg(imageName));
        return false;
    }

    // taskkill returns non-zero when the image is not running; that's acceptable.
    if (!(killer.exitCode() == 0 || killer.exitCode() == 128))
    {
        AppLogger::error(QStringLiteral("taskkill failed for image %1 with code %2").arg(imageName).arg(killer.exitCode()));
        return false;
    }

    AppLogger::info(QStringLiteral("taskkill finished for image %1 with code %2").arg(imageName).arg(killer.exitCode()));
    return true;
}

bool Rpcs3SessionController::createProcessDetached(const QString& commandLine,
                                                   const QString& workingDirectory,
                                                   DWORD* outPid,
                                                   QString* outErrorDetail)
{
    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo{};

    std::wstring cmd = commandLine.toStdWString();
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');

    const std::wstring workingDir = QDir::toNativeSeparators(workingDirectory).toStdWString();
    const wchar_t* workingDirPtr = workingDir.empty() ? nullptr : workingDir.c_str();

    const BOOL ok = CreateProcessW(nullptr,
                                   mutableCmd.data(),
                                   nullptr,
                                   nullptr,
                                   FALSE,
                                   CREATE_UNICODE_ENVIRONMENT,
                                   nullptr,
                                   workingDirPtr,
                                   &startupInfo,
                                   &processInfo);

    if (!ok)
    {
        const DWORD code = GetLastError();
        if (outErrorDetail != nullptr)
        {
            *outErrorDetail = QStringLiteral("CreateProcessW failed (win32=%1)").arg(code);
        }
        return false;
    }

    if (outPid != nullptr)
    {
        *outPid = processInfo.dwProcessId;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

bool Rpcs3SessionController::waitForProcessStability(DWORD pid,
                                                      int timeoutMs,
                                                      DWORD* outExitCode,
                                                      QString* outErrorDetail)
{
    if (timeoutMs < 0)
    {
        timeoutMs = 0;
    }

    HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process == nullptr)
    {
        const DWORD code = GetLastError();
        if (outErrorDetail != nullptr)
        {
            *outErrorDetail = QStringLiteral("OpenProcess failed while checking launch stability (win32=%1)").arg(code);
        }
        return false;
    }

    const DWORD waitResult = WaitForSingleObject(process, static_cast<DWORD>(timeoutMs));
    if (waitResult == WAIT_TIMEOUT)
    {
        CloseHandle(process);
        return true;
    }

    if (waitResult != WAIT_OBJECT_0)
    {
        const DWORD code = GetLastError();
        CloseHandle(process);
        if (outErrorDetail != nullptr)
        {
            *outErrorDetail = QStringLiteral("WaitForSingleObject failed while checking launch stability (win32=%1)").arg(code);
        }
        return false;
    }

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(process, &exitCode))
    {
        const DWORD code = GetLastError();
        CloseHandle(process);
        if (outErrorDetail != nullptr)
        {
            *outErrorDetail = QStringLiteral("GetExitCodeProcess failed (win32=%1)").arg(code);
        }
        return false;
    }

    CloseHandle(process);
    if (outExitCode != nullptr)
    {
        *outExitCode = exitCode;
    }

    return false;
}

bool Rpcs3SessionController::waitForProcessExit(DWORD pid, int timeoutMs)
{
    if (timeoutMs < 0)
    {
        timeoutMs = 0;
    }

    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (process == nullptr)
    {
        return true;
    }

    const DWORD result = WaitForSingleObject(process, static_cast<DWORD>(timeoutMs));
    CloseHandle(process);
    return result == WAIT_OBJECT_0;
}

bool Rpcs3SessionController::restartGame(int restartDelayMs, QString* error) const
{
    if (!validateEmulatorPath(error) || !validateGamePath(error))
    {
        return false;
    }

    if (!ensureNoRunningRpcs3(error))
    {
        return false;
    }

    if (restartDelayMs < 0)
    {
        restartDelayMs = 0;
    }

    ::Sleep(static_cast<DWORD>(restartDelayMs));

    if (!launchGameDetached(error))
    {
        return false;
    }

    if (!waitForRpcs3Start(5000))
    {
        AppLogger::error(QStringLiteral("Restart Game failed: RPCS3 process not detected after launch."));
        return setError(error, QStringLiteral("Restart Game launched but RPCS3 was not running afterward. Check logs for details."));
    }

    return true;
}

QStringList Rpcs3SessionController::processNameCandidates() const
{
    std::set<QString> unique;
    const QString configuredExeName = QFileInfo(emulatorPath_).fileName().trimmed();
    if (!configuredExeName.isEmpty())
    {
        unique.insert(configuredExeName.toLower());
    }

    unique.insert(QStringLiteral("rpcs3.exe"));

    QStringList names;
    for (const QString& name : unique)
    {
        names.push_back(name);
    }
    return names;
}

bool Rpcs3SessionController::ensureNoRunningRpcs3(QString* error) const
{
    const QStringList names = processNameCandidates();
    AppLogger::info(QStringLiteral("RPCS3 cleanup candidates: %1").arg(names.join(QStringLiteral(", "))));

    for (const QString& name : names)
    {
        if (!killProcessImage(name))
        {
            return setError(error, QStringLiteral("Failed to terminate existing RPCS3 processes."));
        }
    }

    constexpr int kTimeoutMs = 10000;
    constexpr int kPollMs = 100;
    int waitedMs = 0;

    while (waitedMs <= kTimeoutMs)
    {
        bool anyRunning = false;
        QStringList stillRunning;
        for (const QString& name : names)
        {
            if (!findProcessIdsByExeName(name.toStdWString()).empty())
            {
                anyRunning = true;
                stillRunning.push_back(name);
            }
        }

        if (!anyRunning)
        {
            AppLogger::info(QStringLiteral("RPCS3 cleanup complete; no processes running."));
            return true;
        }

        if (waitedMs % 1000 == 0)
        {
            AppLogger::warn(QStringLiteral("Waiting for RPCS3 processes to exit (%1 ms): %2")
                                .arg(waitedMs)
                                .arg(stillRunning.join(QStringLiteral(", "))));
        }

        ::Sleep(kPollMs);
        waitedMs += kPollMs;
    }

    AppLogger::error(QStringLiteral("Timeout waiting for RPCS3 cleanup."));
    return setError(error, QStringLiteral("Timed out waiting for existing RPCS3 processes to exit."));
}

bool Rpcs3SessionController::waitForRpcs3Start(int timeoutMs) const
{
    if (timeoutMs < 0)
    {
        timeoutMs = 0;
    }

    constexpr int kPollMs = 100;
    int waitedMs = 0;
    while (waitedMs <= timeoutMs)
    {
        if (findRunningPid().has_value())
        {
            return true;
        }

        ::Sleep(kPollMs);
        waitedMs += kPollMs;
    }

    return false;
}

bool Rpcs3SessionController::launchEmulatorDetached(QString* error) const
{
    const QString normalizedEmuPath = QDir::toNativeSeparators(emulatorPath_);
    const QString workingDir = QFileInfo(normalizedEmuPath).absolutePath();
    const QString commandLine = QStringLiteral("\"%1\"").arg(normalizedEmuPath);
    AppLogger::info(QStringLiteral("Launching RPCS3: %1 (cwd=%2)").arg(commandLine, workingDir));

    DWORD pid = 0;
    QString errorDetail;
    if (!createProcessDetached(commandLine, workingDir, &pid, &errorDetail))
    {
        AppLogger::error(QStringLiteral("Launch RPCS3 failed: %1").arg(errorDetail));
        return setError(error, QStringLiteral("Failed to start RPCS3."));
    }

    DWORD exitCode = 0;
    QString stabilityError;
    if (!waitForProcessStability(pid, 2000, &exitCode, &stabilityError))
    {
        if (!stabilityError.isEmpty())
        {
            AppLogger::error(QStringLiteral("Launch RPCS3 stability check failed for pid=%1: %2").arg(pid).arg(stabilityError));
            return setError(error, QStringLiteral("RPCS3 launch verification failed."));
        }

        AppLogger::error(QStringLiteral("Launch RPCS3 exited quickly. pid=%1 exitCode=%2").arg(pid).arg(exitCode));
        return setError(error, QStringLiteral("RPCS3 exited immediately (code %1). Check logs.").arg(exitCode));
    }

    AppLogger::info(QStringLiteral("Launch RPCS3 detached started with pid=%1.").arg(pid));

    return true;
}

bool Rpcs3SessionController::launchGameDetached(QString* error) const
{
    const QString normalizedEmuPath = QDir::toNativeSeparators(emulatorPath_);
    const QString normalizedGamePath = QDir::toNativeSeparators(gamePath_);
    const QString workingDir = QFileInfo(normalizedEmuPath).absolutePath();
    const QString commandLine = QStringLiteral("\"%1\" --no-gui \"%2\"")
                                    .arg(normalizedEmuPath, normalizedGamePath);
    AppLogger::info(QStringLiteral("Launching Game: %1 (cwd=%2)").arg(commandLine, workingDir));

    DWORD pid = 0;
    QString errorDetail;
    if (!createProcessDetached(commandLine, workingDir, &pid, &errorDetail))
    {
        AppLogger::error(QStringLiteral("Launch Game failed: %1").arg(errorDetail));
        return setError(error, QStringLiteral("Failed to start RPCS3 with the configured game target."));
    }

    DWORD exitCode = 0;
    QString stabilityError;
    if (!waitForProcessStability(pid, 2000, &exitCode, &stabilityError))
    {
        if (!stabilityError.isEmpty())
        {
            AppLogger::error(QStringLiteral("Launch Game stability check failed for pid=%1: %2").arg(pid).arg(stabilityError));
            return setError(error, QStringLiteral("Start Game launch verification failed."));
        }

        AppLogger::error(QStringLiteral("Launch Game exited quickly. pid=%1 exitCode=%2").arg(pid).arg(exitCode));
        return setError(error, QStringLiteral("Start Game exited immediately (code %1). Check logs.").arg(exitCode));
    }

    AppLogger::info(QStringLiteral("Launch Game detached started with pid=%1.").arg(pid));

    return true;
}
