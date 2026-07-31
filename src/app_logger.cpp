#include "app_logger.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QTextStream>

namespace
{
QMutex gLogMutex;

QString buildLogPath()
{
    QString baseDir = QCoreApplication::applicationDirPath();
    if (baseDir.trimmed().isEmpty())
    {
        baseDir = QDir::currentPath();
    }

    QDir dir(baseDir);
    dir.mkpath(QStringLiteral("."));
    dir.mkpath(QStringLiteral("logs"));
    return dir.filePath(QStringLiteral("logs/trr_qt_trainer.log"));
}

void rotateIfLarge(const QString& path)
{
    QFileInfo info(path);
    constexpr qint64 kMaxBytes = 2 * 1024 * 1024;
    if (!info.exists() || info.size() < kMaxBytes)
    {
        return;
    }

    const QString backup = path + QStringLiteral(".1");
    QFile::remove(backup);
    QFile::rename(path, backup);
}

void writeLogLine(const QString& level, const QString& message)
{
    QMutexLocker lock(&gLogMutex);

    const QString path = buildLogPath();
    rotateIfLarge(path);

    QFile logFile(path);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return;
    }

    QTextStream stream(&logFile);
    stream.setEncoding(QStringConverter::Utf8);
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    stream << ts << " [" << level << "] " << message << "\n";
    stream.flush();
}
}

namespace AppLogger
{
void initialize()
{
    writeLogLine(QStringLiteral("INFO"), QStringLiteral("Logger initialized"));
}

QString logFilePath()
{
    return buildLogPath();
}

void info(const QString& message)
{
    writeLogLine(QStringLiteral("INFO"), message);
}

void warn(const QString& message)
{
    writeLogLine(QStringLiteral("WARN"), message);
}

void error(const QString& message)
{
    writeLogLine(QStringLiteral("ERROR"), message);
}
}
