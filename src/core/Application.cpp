// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Application.h"

#include <config.h>

#include <QDir>
#include <QDirIterator>
#include <QFontDatabase>

#include <exception>

#include "OutOfMemoryHandler.h"

Application::Application(int& argc, char** argv) : QApplication(argc, argv), m_currentLocale("en") {
  initTranslations();
  loadFonts();
}

bool Application::notify(QObject* receiver, QEvent* e) {
  try {
    return QApplication::notify(receiver, e);
  } catch (const std::bad_alloc&) {
    OutOfMemoryHandler::instance().handleOutOfMemorySituation();
    return false;
  } catch (const std::exception& e) {
    qCritical("Unhandled exception while dispatching a Qt event: %s", e.what());
    return false;
  } catch (...) {
    qCritical("Unhandled unknown exception while dispatching a Qt event.");
    return false;
  }
}

void Application::installLanguage(const QString& locale) {
  if (m_currentLocale == locale) {
    return;
  }

  if (m_translationsMap.find(locale) != m_translationsMap.end()) {
    bool loaded = m_translator.load(m_translationsMap[locale]);

    QCoreApplication::removeTranslator(&m_translator);
    QCoreApplication::installTranslator(&m_translator);

    m_currentLocale = (loaded) ? locale : "en";
  } else {
    QCoreApplication::removeTranslator(&m_translator);

    m_currentLocale = "en";
  }
}

const QString& Application::getCurrentLocale() const {
  return m_currentLocale;
}

std::list<QString> Application::getLanguagesList() const {
  std::list<QString> list{"en"};
  std::transform(m_translationsMap.begin(), m_translationsMap.end(), std::back_inserter(list),
                 [](const std::pair<QString, QString>& val) { return val.first; });
  return list;
}

void Application::initTranslations() {
  auto opt = Qt::SkipEmptyParts;
  const QStringList translationDirs(QString::fromUtf8(TRANSLATION_DIRS).split(QChar(':'), opt));

  const QStringList languageFileFilter("scantailor_*.qm");
  for (const QString& path : translationDirs) {
    QDir dir = (QDir::isAbsolutePath(path)) ? QDir(path) : QDir::cleanPath(applicationDirPath() + '/' + path);
    if (dir.exists()) {
      QStringList translationFileNames = QDir(dir.path()).entryList(languageFileFilter);
      for (const QString& fileName : translationFileNames) {
        QString locale(fileName);
        locale.truncate(locale.lastIndexOf('.'));
        locale.remove(0, locale.indexOf('_') + 1);

        m_translationsMap[locale] = dir.absoluteFilePath(fileName);
      }
    }
  }
}

void Application::loadFonts() {
  QDirIterator it(":/fonts");
  while (it.hasNext()) {
    QFontDatabase::addApplicationFont(it.next());
  }
}
