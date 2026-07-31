#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <optional>
#include <vector>

#define NOMINMAX
#include <Windows.h>

class Rpcs3SessionController
{
public:
    void setEmulatorPath(const QString& path);
    void setGamePath(const QString& path);

    QString emulatorPath() const;
    QString gamePath() const;

    std::optional<DWORD> findRunningPid() const;

    bool startEmulator(QString* error = nullptr) const;
    bool resetEmulator(int restartDelayMs = 500, QString* error = nullptr) const;
    bool stopEmulatorSession(QString* error = nullptr) const;
    bool startGame(QString* error = nullptr) const;
    bool restartGame(int restartDelayMs = 500, QString* error = nullptr) const;

private:
    static std::optional<DWORD> findProcessIdByExeName(const std::wstring& exeName);
    static std::vector<DWORD> findProcessIdsByExeName(const std::wstring& exeName);
    static bool killProcessTree(DWORD pid);
    static bool killProcessImage(const QString& imageName);
    static bool createProcessDetached(const QString& commandLine,
                                      const QString& workingDirectory,
                                      DWORD* outPid,
                                      QString* outErrorDetail);
    static bool waitForProcessStability(DWORD pid,
                                        int timeoutMs,
                                        DWORD* outExitCode,
                                        QString* outErrorDetail);
    static bool waitForProcessExit(DWORD pid, int timeoutMs);
    static bool setError(QString* outError, const QString& message);

    QStringList processNameCandidates() const;
    bool ensureNoRunningRpcs3(QString* error) const;
    bool waitForRpcs3Start(int timeoutMs) const;
    bool launchEmulatorDetached(QString* error) const;
    bool launchGameDetached(QString* error) const;
    bool validateEmulatorPath(QString* error) const;
    bool validateGamePath(QString* error) const;

    QString emulatorPath_;
    QString gamePath_;
};
