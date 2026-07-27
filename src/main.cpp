#include <locale.h>

#include <QGuiApplication>
#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QtQml>
#include <QtWebEngine/qtwebengineglobal.h>
#include <QErrorMessage>
#include <QCommandLineOption>

#include "shared/Names.h"
#include "system/SystemComponent.h"
#include "QsLog.h"
#include "Paths.h"
#include "player/CodecsComponent.h"
#include "player/PlayerComponent.h"
#include "player/OpenGLDetect.h"
#include "Version.h"
#include "settings/SettingsComponent.h"
#include "settings/SettingsSection.h"
#include "ui/KonvergoWindow.h"
#include "Globals.h"
#include "ui/ErrorMessage.h"
#include "UniqueApplication.h"
#include "utils/Log.h"

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
#include "SignalManager.h"
#endif

/////////////////////////////////////////////////////////////////////////////////////////
static void preinitQt()
{
  QCoreApplication::setApplicationName(Names::MainName());
  QCoreApplication::setApplicationVersion(Version::GetVersionString());
  QCoreApplication::setOrganizationDomain("jellyfin.org");

#ifdef Q_OS_WIN32
  QVariant useOpengl = SettingsComponent::readPreinitValue(SETTINGS_SECTION_MAIN, "useOpenGL");

  // Warning: this must be the same as the default value as declared in
  // the settings_description.json file, or confusion will result.
  if (useOpengl.type() != QMetaType::Bool)
    useOpengl = false;

  if (useOpengl.toBool())
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
  else
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
char** appendCommandLineArguments(int argc, char **argv, const QStringList& args)
{
  size_t newSize = (argc + args.length() + 1) * sizeof(char*);
  char** newArgv = (char**)calloc(1, newSize);
  memcpy(newArgv, argv, (size_t)(argc * sizeof(char*)));

  int pos = argc;
  for(const QString& str : args)
    newArgv[pos++] = qstrdup(str.toUtf8().data());

  return newArgv;
}

/////////////////////////////////////////////////////////////////////////////////////////
void ShowLicenseInfo()
{
  QFile licenses(":/misc/licenses.txt");
  licenses.open(QIODevice::ReadOnly | QIODevice::Text);
  QByteArray contents = licenses.readAll();
  printf("%.*s\n", contents.size(), contents.data());
}

/////////////////////////////////////////////////////////////////////////////////////////
QStringList g_qtFlags = {
  "--enable-gpu-rasterization"
};

/////////////////////////////////////////////////////////////////////////////////////////
QString commandLineValue(const QStringList& arguments, const QString& option,
                         const QString& defaultValue)
{
  const QString assignmentPrefix = option + "=";
  for (int i = 0; i < arguments.size(); i++)
  {
    if (arguments[i].startsWith(assignmentPrefix))
      return arguments[i].mid(assignmentPrefix.size());

    if (arguments[i] == option && i + 1 < arguments.size())
      return arguments[i + 1];
  }

  return defaultValue;
}

/////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
  try
  {
    QCommandLineParser parser;
    parser.setApplicationDescription("Terminus Player");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({{{"l", "licenses"},         "Show license information"},
                       {"desktop",                 "Start in desktop mode"},
                       {"tv",                      "Start in TV mode"},
                       {"windowed",                "Start in windowed mode"},
                       {"fullscreen",              "Start in fullscreen"},
                       {"terminal",                "Log to terminal"},
                       {"disable-gpu",             "Disable QtWebEngine gpu accel"}});

    auto scaleOption = QCommandLineOption("scale-factor", "Set to a integer or default auto which controls" \
                                                          "the scale (DPI) of the desktop interface.");
    scaleOption.setValueName("scale");
    scaleOption.setDefaultValue("auto");
    
    auto devOption = QCommandLineOption("remote-debugging-port", "Port number for devtools.");
    devOption.setValueName("port");
    parser.addOption(scaleOption);
    parser.addOption(devOption);

    QStringList arguments;
    for (int i = 0; i < argc; i++)
      arguments << QString::fromLocal8Bit(argv[i]);

    char **newArgv = appendCommandLineArguments(argc, argv, g_qtFlags);
    int newArgc = argc + g_qtFlags.size();

    // Qt calls setlocale(LC_ALL, "") in a bunch of places, which breaks
    // float/string processing in mpv and ffmpeg.
#ifdef Q_OS_UNIX
    qputenv("LC_ALL", "C");
    qputenv("LC_NUMERIC", "C");
#endif

    preinitQt();
    auto scale = commandLineValue(arguments, "--scale-factor", "auto");
    if (scale.isEmpty() || scale == "auto")
      QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    else if (scale != "none")
      qputenv("QT_SCALE_FACTOR", scale.toUtf8());

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    detectOpenGLEarly();
    QtWebEngine::initialize();

    QApplication app(newArgc, newArgv);
    app.setApplicationName("Terminus Player");
    parser.process(arguments);

    if (parser.isSet("licenses"))
    {
      ShowLicenseInfo();
      return EXIT_SUCCESS;
    }

#if defined(Q_OS_WIN) 
    // Setting window icon on OSX will break user ability to change it
    app.setWindowIcon(QIcon(":/images/icon.png"));
#endif

#if defined(Q_OS_LINUX)
  	// Set window icon on Linux using system icon theme
  	app.setWindowIcon(QIcon::fromTheme("com.github.iwalton3.jellyfin-media-player", QIcon(":/images/icon.png")));
#endif

    UniqueApplication* uniqueApp = new UniqueApplication();
    if (!uniqueApp->ensureUnique())
      return EXIT_SUCCESS;

#ifdef Q_OS_UNIX
    // install signals handlers for proper app closing.
    SignalManager signalManager(&app);
    Q_UNUSED(signalManager);
#endif

    Log::Init();
    if (parser.isSet("terminal"))
      Log::EnableTerminalOutput();

    detectOpenGLLate();

    Codecs::preinitCodecs();

    // Initialize all the components. This needs to be done
    // early since most everything else relies on it
    //
    ComponentManager::Get().initialize();

    SettingsComponent::Get().setCommandLineValues(parser.optionNames());

    // load QtWebChannel so that we can register our components with it.
    QQmlApplicationEngine *engine = Globals::Engine();

    KonvergoWindow::RegisterClass();
    Globals::SetContextProperty("components", &ComponentManager::Get().getQmlPropertyMap());

    // the only way to detect if QML parsing fails is to hook to this signal and then see
    // if we get a valid object passed to it. Any error messages will be reported on stderr
    // but since no normal user should ever see this it should be fine
    //
    QObject::connect(engine, &QQmlApplicationEngine::objectCreated, [=](QObject* object, const QUrl& url)
    {
      Q_UNUSED(url);

      if (object == nullptr)
        throw FatalException(QObject::tr("Failed to parse application engine script."));

      KonvergoWindow* window = Globals::MainWindow();

      QObject* webChannel = qvariant_cast<QObject*>(window->property("webChannel"));
      Q_ASSERT(webChannel);
      ComponentManager::Get().setWebChannel(qobject_cast<QWebChannel*>(webChannel));

      QObject::connect(uniqueApp, &UniqueApplication::otherApplicationStarted, window, &KonvergoWindow::otherAppFocus);
    });
    engine->load(QUrl(QStringLiteral("qrc:/ui/webview.qml")));

    Log::UpdateLogLevel();

    // run our application
    int ret = app.exec();

    delete uniqueApp;
    Globals::EngineDestroy();

    Codecs::Uninit();
    Log::Uninit();
    return ret;
  }
  catch (FatalException& e)
  {
    QLOG_FATAL() << "Unhandled FatalException:" << qPrintable(e.message());
    QApplication errApp(argc, argv);

    auto  msg = new ErrorMessage(e.message(), true);
    msg->show();

    errApp.exec();

    Codecs::Uninit();
    Log::Uninit();
    return 1;
  }
}
