#pragma once

#include <QtCore/QString>

namespace AppLogger
{
void initialize();
QString logFilePath();
void info(const QString& message);
void warn(const QString& message);
void error(const QString& message);
}
