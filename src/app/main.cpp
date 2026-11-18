// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <config.h>
#include <core/Application.h>
#include <core/ApplicationSettings.h>
#include <core/ColorSchemeFactory.h>
#include <core/ColorSchemeManager.h>
#include <core/FontIconPack.h>
#include <core/IconProvider.h>
#include <core/StyledIconPack.h>

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStringList>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
#if QT_VERSION_MAJOR == 5
  // Issue #97: Qt5 on Wayland can corrupt dialogs; use X11 unless opted out.
  if (!qEnvironmentVariableIsSet("SCANTAILOR_NO_XCB_FALLBACK")) {
    if (qgetenv("XDG_SESSION_TYPE") == "wayland" && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
      qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));
    }
  }
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  // Issue #99: better quality with fractional UI scaling (125%, 150%, etc.)
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
#endif
  Application app(argc, argv);
  Application::setApplicationName(APPLICATION_NAME);
  Application::setOrganizationName(ORGANIZATION_NAME);

  QCommandLineParser parser;
  parser.setApplicationDescription(QObject::tr(
      "ScanTailor Advanced. Specify a project file to open it, or an existing directory "
      "to use as the initial input/output location."));
  parser.addHelpOption();
  parser.addPositionalArgument(
      "path",
      QObject::tr("Optional project file (.ScanTailor) or initial directory."));
  parser.process(app);

  QString initialDirectory;
  QString projectFileToOpen;
  const QStringList positionalArgs = parser.positionalArguments();
  if (positionalArgs.size() > 1) {
    parser.showMessageAndExit(QCommandLineParser::MessageType::Error,
                              QObject::tr("Specify at most one project file or directory."),
                              2);
  }
  if (positionalArgs.size() == 1) {
    const QString path = positionalArgs.at(0);
    const QFileInfo info(QDir::cleanPath(path));
    if (info.isRelative()) {
      const QFileInfo absoluteInfo(QDir(QDir::currentPath()).absoluteFilePath(info.filePath()));
      if (absoluteInfo.exists()) {
        if (absoluteInfo.isDir()) {
          initialDirectory = absoluteInfo.absoluteFilePath();
        } else {
          projectFileToOpen = absoluteInfo.absoluteFilePath();
          initialDirectory = absoluteInfo.absolutePath();
        }
      } else {
        parser.showMessageAndExit(
            QCommandLineParser::MessageType::Error,
            QObject::tr("Path does not exist: %1").arg(info.absoluteFilePath()),
            2);
      }
    } else if (info.exists()) {
      if (info.isDir()) {
        initialDirectory = info.absoluteFilePath();
      } else {
        projectFileToOpen = info.absoluteFilePath();
        initialDirectory = info.absolutePath();
      }
    } else {
      parser.showMessageAndExit(
          QCommandLineParser::MessageType::Error,
          QObject::tr("Path does not exist: %1").arg(info.absoluteFilePath()),
          2);
    }
  }

#ifdef _WIN32
  // Get rid of all references to Qt's installation directory.
  Application::setLibraryPaths(QStringList(Application::applicationDirPath()));
#endif

  QSettings::setDefaultFormat(QSettings::IniFormat);
  if (app.isPortableVersion()) {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, app.getPortableConfigPath());
  }
  QSettings settings;

  app.installLanguage(ApplicationSettings::getInstance().getLanguage());

  {
    std::unique_ptr<ColorScheme> scheme
        = ColorSchemeFactory().create(ApplicationSettings::getInstance().getColorScheme());
    ColorSchemeManager::instance().setColorScheme(*scheme);
  }
  IconProvider::getInstance().setIconPack(StyledIconPack::createDefault());

  auto* mainWnd = new MainWindow(initialDirectory);
  mainWnd->setAttribute(Qt::WA_DeleteOnClose);
  if (settings.value("mainWindow/maximized") == false) {
    mainWnd->show();
  } else {
    // mainWnd->showMaximized();  // Doesn't work for Windows.
    QTimer::singleShot(0, mainWnd, &QMainWindow::showMaximized);
  }

  if (!projectFileToOpen.isEmpty()) {
    mainWnd->openProject(projectFileToOpen);
  }
  return Application::exec();
}  // main
