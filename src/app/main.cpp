// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <config.h>
#include <core/Application.h>
#include <core/ApplicationSettings.h>
#include <core/ColorSchemeFactory.h>
#include <core/ColorSchemeManager.h>
#include <core/FontIconPack.h>
#include <core/IconProvider.h>
#include <core/ImageAllocationLimit.h>
#include <core/PathUtils.h>
#include <core/SettingsStore.h>
#include <core/StyledIconPack.h>

#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QStringList>
#include <QTextStream>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
#ifdef Q_OS_LINUX
  // Qt 6.12 can load GTK3's platform theme plugin even for Qt-owned dialogs.
  // With mismatched libpng builds this crashes in gdk_pixbuf/png_destroy_read_struct.
  // Keep the process on Qt's generic theme unless the user explicitly overrides it.
  if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORMTHEME")) {
    qputenv("QT_QPA_PLATFORMTHEME", QByteArrayLiteral("generic"));
  }
#endif
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
  Application app(argc, argv);
  Application::setApplicationName(APPLICATION_NAME);
  Application::setApplicationDisplayName(APPLICATION_DISPLAY_NAME);
  Application::setOrganizationName(ORGANIZATION_NAME);

  QCommandLineParser parser;
  parser.setApplicationDescription(
      QObject::tr("%1. Specify a project file or an existing directory to use as the initial "
                  "input/output location. The default configuration directory is %2.")
          .arg(QString::fromUtf8(APPLICATION_DISPLAY_NAME), core::toQString(SettingsStore::defaultConfigDirectory())));
  parser.addHelpOption();
  const QCommandLineOption configOption({"c", "config"},
                                        QObject::tr("Store settings in this directory (config.toml is created "
                                                    "directly inside it)."),
                                        QObject::tr("directory"));
  parser.addOption(configOption);
  parser.addPositionalArgument("path", QObject::tr("Optional project file (.ScanTailor) or initial directory."));
  parser.process(app);

  QString initialDirectory;
  QString projectFileToOpen;
  const QStringList positionalArgs = parser.positionalArguments();
  if (positionalArgs.size() > 1) {
    QTextStream errorStream(stderr);
    errorStream << QObject::tr("Specify at most one project file or directory.") << '\n';
    return 2;
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
        QTextStream errorStream(stderr);
        errorStream << QObject::tr("Path does not exist: %1").arg(info.absoluteFilePath()) << '\n';
        return 2;
      }
    } else if (info.exists()) {
      if (info.isDir()) {
        initialDirectory = info.absoluteFilePath();
      } else {
        projectFileToOpen = info.absoluteFilePath();
        initialDirectory = info.absolutePath();
      }
    } else {
      QTextStream errorStream(stderr);
      errorStream << QObject::tr("Path does not exist: %1").arg(info.absoluteFilePath()) << '\n';
      return 2;
    }
  }

#ifdef _WIN32
  // Get rid of all references to Qt's installation directory.
  Application::setLibraryPaths(QStringList(Application::applicationDirPath()));
#endif

  QString configError;
  const bool explicitConfigDirectory = parser.isSet(configOption);
  if (!SettingsStore::initialize(parser.value(configOption), explicitConfigDirectory, &configError,
                                 /*preserveExistingSettings=*/!explicitConfigDirectory)) {
    QTextStream errorStream(stderr);
    errorStream << configError << '\n';
    QMessageBox::critical(nullptr, QObject::tr("Configuration error"), configError);
    return 2;
  }
  if (!configError.isEmpty()) {
    qWarning().noquote() << configError;
    if (!explicitConfigDirectory) {
      QMessageBox::warning(nullptr, QObject::tr("Configuration warning"), configError);
    }
  }
  SettingsStore::setWriteErrorHandler([&app](const QString& message) {
    QMetaObject::invokeMethod(
        &app, [message]() { QMessageBox::critical(nullptr, QObject::tr("Configuration error"), message); },
        Qt::QueuedConnection);
  });
  SettingsStore settings;

  ImageAllocationLimit::defaultLimitMb();
  if (!ImageAllocationLimit::hasEnvironmentOverride()) {
    const int imageAllocationLimitMb = ApplicationSettings::getInstance().getImageAllocationLimitMb();
    if (imageAllocationLimitMb > 0) {
      int effectiveLimitMb = 0;
      if (ImageAllocationLimit::setLimitMb(imageAllocationLimitMb, &effectiveLimitMb)
          != ImageAllocationLimit::UpdateResult::Applied) {
        qWarning() << "The saved image allocation limit was rejected.";
      }
    }
  }

  app.installLanguage(ApplicationSettings::getInstance().getLanguage());

  {
    std::unique_ptr<ColorScheme> scheme
        = ColorSchemeFactory().create(ApplicationSettings::getInstance().getColorScheme());
    ColorSchemeManager::instance().setColorScheme(*scheme);
  }
  IconProvider::getInstance().setIconPack(StyledIconPack::createDefault());

  auto* mainWnd = new MainWindow(core::toFileSystemPath(initialDirectory));
  mainWnd->setAttribute(Qt::WA_DeleteOnClose);
  if (!settings.boolValue("mainWindow/maximized", true)) {
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
