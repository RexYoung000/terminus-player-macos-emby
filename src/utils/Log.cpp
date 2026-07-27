//
// Created by Tobias Hieta on 07/03/16.
//

#include "Log.h"

#include <QtQml>
#include <QGuiApplication>

#include "QsLog.h"
#include "shared/Names.h"
#include "shared/Paths.h"
#include "settings/SettingsComponent.h"
#include "LogRedaction.h"
#include "Version.h"

using namespace QsLogging;

/////////////////////////////////////////////////////////////////////////////////////////
static void qtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QString prefix;
    if (context.line)
      prefix = QString("%1:%2:%3: ").arg(context.file).arg(context.line).arg(context.function);
    QString text = prefix + msg;
    switch (type)
    {
      case QtDebugMsg:
        QLOG_DEBUG() << text;
        break;
      case QtInfoMsg:
        QLOG_INFO() << text;
        break;
      case QtWarningMsg:
        QLOG_WARN() << text;
        break;
      case QtCriticalMsg:
        QLOG_ERROR() << text;
        break;
      case QtFatalMsg:
        QLOG_FATAL() << text;
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
void Log::CensorAuthTokens(QString& msg)
{
  LogRedaction::CensorAuthTokens(msg);
}

/////////////////////////////////////////////////////////////////////////////////////////
static QsLogging::Level logLevelFromString(const QString& str)
{
  if (str == "trace")     return QsLogging::Level::TraceLevel;
  if (str == "debug")     return QsLogging::Level::DebugLevel;
  if (str == "info")      return QsLogging::Level::InfoLevel;
  if (str == "warn")      return QsLogging::Level::WarnLevel;
  if (str == "error")     return QsLogging::Level::ErrorLevel;
  if (str == "fatal")     return QsLogging::Level::FatalLevel;
  if (str == "disable")   return QsLogging::Level::OffLevel;
  // if not valid, use default
  return QsLogging::Level::DebugLevel;
}

/////////////////////////////////////////////////////////////////////////////////////////
void Log::UpdateLogLevel()
{
  QString level = SettingsComponent::Get().value(SETTINGS_SECTION_MAIN, "logLevel").toString();
  if (level.size())
  {
    QLOG_INFO() << "Setting log level to:" << level;
    Logger::instance().setLoggingLevel(logLevelFromString(level));
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
void Log::Init()
{
  // Note where the logfile is going to be
  qDebug("Logging to %s", qPrintable(Paths::logDir(Names::MainName() + ".log")));

  // init logging.
  DestinationPtr dest = DestinationFactory::MakeFileDestination(
    Paths::logDir(Names::MainName() + ".log"),
    EnableLogRotationOnOpen,
    MaxSizeBytes(1024 * 1024),
    MaxOldLogCount(9));

  Logger::instance().addDestination(dest);
  Logger::instance().setLoggingLevel(DebugLevel);
  Logger::instance().setProcessingCallback(Log::CensorAuthTokens);

  qInstallMessageHandler(qtMessageOutput);

  QLOG_INFO() << "Starting Terminus Player version:" << qPrintable(Version::GetVersionString()) << "build date:" << qPrintable(Version::GetBuildDate());
  QLOG_INFO() << qPrintable(QString("  Running on: %1 [%2] arch %3").arg(QSysInfo::prettyProductName()).arg(QSysInfo::kernelVersion()).arg(QSysInfo::currentCpuArchitecture()));
  QLOG_INFO() << "  Qt Version:" << QT_VERSION_STR << qPrintable(QString("[%1]").arg(QSysInfo::buildAbi()));
}

/////////////////////////////////////////////////////////////////////////////////////////
void Log::EnableTerminalOutput()
{
  Logger::instance().addDestination(DestinationFactory::MakeDebugOutputDestination());
}

/////////////////////////////////////////////////////////////////////////////////////////
void Log::Uninit()
{
  qInstallMessageHandler(0);
  Logger::destroyInstance();
}
