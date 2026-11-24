// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SettingsStore.h"

#include <config.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <atomic>
#include <utility>

namespace {

QString g_configDirectory;
QString g_configFilePath;
bool g_configDirectoryWritable = false;
bool g_configDirectoryExplicit = false;
bool g_initialized = false;
QMutex g_stateMutex;
SettingsStore::WriteErrorHandler g_writeErrorHandler;
std::atomic_bool g_writeErrorReported = false;

QString normalizedDirectory(const QString& path) {
  return QDir::cleanPath(QDir(path).absolutePath());
}

bool probeDirectory(const QString& path, QString* errorMessage) {
  QDir directory(path);
  if (!directory.exists() && !directory.mkpath(".")) {
    if (errorMessage) {
      *errorMessage = QObject::tr("Unable to create the configuration directory: %1").arg(path);
    }
    return false;
  }

  const QFileInfo directoryInfo(directory.absolutePath());
  if (!directoryInfo.isDir()) {
    if (errorMessage) {
      *errorMessage = QObject::tr("The configuration path is not a directory: %1").arg(path);
    }
    return false;
  }

  QTemporaryFile probe(directory.filePath(".scantailor-write-check-XXXXXX"));
  if (!probe.open()) {
    if (errorMessage) {
      *errorMessage
          = QObject::tr("Unable to write to the configuration directory %1: %2").arg(path, probe.errorString());
    }
    return false;
  }
  const QString probePath = probe.fileName();
  probe.close();
  if (!QFile::remove(probePath)) {
    if (errorMessage) {
      *errorMessage = QObject::tr("Unable to remove the configuration write-test file: %1").arg(probePath);
    }
    return false;
  }
  return true;
}

}  // namespace

class SettingsStore::Backend {
 public:
  virtual ~Backend() = default;
  virtual bool boolValue(const QString& key, bool defaultValue) const = 0;
  virtual int intValue(const QString& key, int defaultValue, bool* ok) const = 0;
  virtual double doubleValue(const QString& key, double defaultValue) const = 0;
  virtual QString stringValue(const QString& key, const QString& defaultValue) const = 0;
  virtual QByteArray byteArrayValue(const QString& key, const QByteArray& defaultValue) const = 0;
  virtual QSize sizeValue(const QString& key, const QSize& defaultValue) const = 0;
  virtual QSizeF sizeFValue(const QString& key, const QSizeF& defaultValue) const = 0;
  virtual bool contains(const QString& key) const = 0;
  virtual bool hasError() const = 0;
  virtual QStringList stringList(const QString& key) const = 0;
  virtual void setValue(const QString& key, bool value) = 0;
  virtual void setValue(const QString& key, int value) = 0;
  virtual void setValue(const QString& key, double value) = 0;
  virtual void setValue(const QString& key, const QString& value) = 0;
  virtual void setValue(const QString& key, const QByteArray& value) = 0;
  virtual void setValue(const QString& key, const QSize& value) = 0;
  virtual void setValue(const QString& key, const QSizeF& value) = 0;
  virtual void setStringList(const QString& key, const QStringList& values) = 0;
  virtual void remove(const QString& key) = 0;
  virtual QString sync() = 0;
};

namespace {

class IniBackend final : public SettingsStore::Backend {
 public:
  explicit IniBackend(const QString& filePath) : m_settings(filePath, QSettings::IniFormat) {}

  static QString legacyFilePath() {
    const QString applicationName = QString::fromUtf8(APPLICATION_NAME);
    const QSettings legacySettings(QSettings::IniFormat, QSettings::UserScope, applicationName, applicationName);
    return legacySettings.fileName();
  }

  bool boolValue(const QString& key, bool defaultValue) const override {
    return m_settings.value(key, defaultValue).toBool();
  }

  int intValue(const QString& key, int defaultValue, bool* ok) const override {
    return m_settings.value(key, defaultValue).toInt(ok);
  }

  double doubleValue(const QString& key, double defaultValue) const override {
    return m_settings.value(key, defaultValue).toDouble();
  }

  QString stringValue(const QString& key, const QString& defaultValue) const override {
    return m_settings.value(key, defaultValue).toString();
  }

  QByteArray byteArrayValue(const QString& key, const QByteArray& defaultValue) const override {
    return m_settings.value(key, defaultValue).toByteArray();
  }

  QSize sizeValue(const QString& key, const QSize& defaultValue) const override {
    return m_settings.value(key, defaultValue).toSize();
  }

  QSizeF sizeFValue(const QString& key, const QSizeF& defaultValue) const override {
    return m_settings.value(key, defaultValue).toSizeF();
  }

  bool contains(const QString& key) const override { return m_settings.contains(key); }

  bool hasError() const override { return m_settings.status() != QSettings::NoError; }

  QStringList stringList(const QString& key) const override {
    QStringList values;
    const int size = m_settings.beginReadArray(key);
    for (int index = 0; index < size; ++index) {
      m_settings.setArrayIndex(index);
      values.push_back(m_settings.value(QStringLiteral("path")).toString());
    }
    m_settings.endArray();
    return values;
  }

  void setValue(const QString& key, bool value) override { m_settings.setValue(key, value); }

  void setValue(const QString& key, int value) override { m_settings.setValue(key, value); }

  void setValue(const QString& key, double value) override { m_settings.setValue(key, value); }

  void setValue(const QString& key, const QString& value) override { m_settings.setValue(key, value); }

  void setValue(const QString& key, const QByteArray& value) override { m_settings.setValue(key, value); }

  void setValue(const QString& key, const QSize& value) override { m_settings.setValue(key, value); }

  void setValue(const QString& key, const QSizeF& value) override { m_settings.setValue(key, value); }

  void setStringList(const QString& key, const QStringList& values) override {
    m_settings.beginWriteArray(key);
    for (int index = 0; index < values.size(); ++index) {
      m_settings.setArrayIndex(index);
      m_settings.setValue(QStringLiteral("path"), values.at(index));
    }
    m_settings.endArray();
  }

  void remove(const QString& key) override { m_settings.remove(key); }

  QString sync() override {
    m_settings.sync();
    switch (m_settings.status()) {
      case QSettings::NoError:
        return QString();
      case QSettings::AccessError:
        return QObject::tr("access to the settings file was denied");
      case QSettings::FormatError:
        return QObject::tr("the settings file has an invalid format");
    }
    return QObject::tr("the settings file could not be saved");
  }

 private:
  mutable QSettings m_settings;
};

void reportWriteError(const QString& path, const QString& error) {
  if (g_writeErrorReported.exchange(true)) {
    return;
  }

  SettingsStore::WriteErrorHandler handler;
  {
    QMutexLocker lock(&g_stateMutex);
    handler = g_writeErrorHandler;
  }
  if (handler) {
    handler(QObject::tr("Unable to save application settings to %1: %2").arg(path, error));
  } else {
    qWarning().noquote() << QObject::tr("Unable to save application settings to %1: %2").arg(path, error);
  }
}

}  // namespace

bool SettingsStore::initialize(const QString& configDirectory,
                               bool explicitDirectory,
                               QString* errorMessage,
                               bool preserveExistingSettings) {
  QMutexLocker lock(&g_stateMutex);
  if (g_initialized) {
    if (errorMessage) {
      errorMessage->clear();
    }
    return true;
  }

  const QString requestedDirectory = configDirectory.isEmpty() ? defaultConfigDirectory() : configDirectory;
  if (requestedDirectory.isEmpty()) {
    if (errorMessage) {
      *errorMessage = QObject::tr("The platform did not provide a configuration directory.");
    }
    return false;
  }
  const QString directory = normalizedDirectory(requestedDirectory);
  QString probeError;
  const bool writable = probeDirectory(directory, &probeError);
  if (!writable && explicitDirectory) {
    if (errorMessage) {
      *errorMessage = probeError;
    }
    return false;
  }

  g_configDirectory = directory;
  g_configFilePath = QDir(directory).filePath(QStringLiteral("config.ini"));
  g_configDirectoryExplicit = explicitDirectory;
  g_configDirectoryWritable = writable;
  g_initialized = true;
  QString initializationWarning = writable ? QString() : probeError;

  if (writable && preserveExistingSettings && !explicitDirectory && !QFileInfo::exists(g_configFilePath)) {
    const QString legacyFilePath = IniBackend::legacyFilePath();
    if (QFileInfo::exists(legacyFilePath) && legacyFilePath != g_configFilePath
        && !QFile::copy(legacyFilePath, g_configFilePath)) {
      initializationWarning = QObject::tr("Unable to copy the existing settings file from %1 to %2.")
                                  .arg(legacyFilePath, g_configFilePath);
    }
  }

  if (errorMessage) {
    *errorMessage = initializationWarning;
  }
  return true;
}

QString SettingsStore::defaultConfigDirectory() {
  const QString root = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
  if (root.isEmpty()) {
    return QString();
  }
  return QDir(root).filePath(QString::fromUtf8(APPLICATION_NAME));
}

QString SettingsStore::configDirectory() {
  if (!g_initialized) {
    initialize(QString(), false);
  }
  return g_configDirectory;
}

QString SettingsStore::configFilePath() {
  if (!g_initialized) {
    initialize(QString(), false);
  }
  return g_configFilePath;
}

QString SettingsStore::profilesDirectory() {
  return QDir(configDirectory()).filePath(QStringLiteral("profiles"));
}

bool SettingsStore::configDirectoryExplicit() {
  if (!g_initialized) {
    initialize(QString(), false);
  }
  return g_configDirectoryExplicit;
}

bool SettingsStore::configDirectoryWritable() {
  if (!g_initialized) {
    initialize(QString(), false);
  }
  return g_configDirectoryWritable;
}

void SettingsStore::setWriteErrorHandler(WriteErrorHandler handler) {
  QMutexLocker lock(&g_stateMutex);
  g_writeErrorHandler = std::move(handler);
}

SettingsStore::SettingsStore() {
  if (!g_initialized) {
    initialize(QString(), false);
  }
  m_filePath = configFilePath();
  m_backend = std::make_unique<IniBackend>(m_filePath);
}

SettingsStore::SettingsStore(const QString& filePath)
    : m_filePath(filePath), m_backend(std::make_unique<IniBackend>(filePath)) {}

SettingsStore::~SettingsStore() = default;
SettingsStore::SettingsStore(SettingsStore&&) noexcept = default;
SettingsStore& SettingsStore::operator=(SettingsStore&&) noexcept = default;

bool SettingsStore::boolValue(const QString& key, bool defaultValue) const {
  return m_backend->boolValue(key, defaultValue);
}

int SettingsStore::intValue(const QString& key, int defaultValue, bool* ok) const {
  return m_backend->intValue(key, defaultValue, ok);
}

double SettingsStore::doubleValue(const QString& key, double defaultValue) const {
  return m_backend->doubleValue(key, defaultValue);
}

QString SettingsStore::stringValue(const QString& key, const QString& defaultValue) const {
  return m_backend->stringValue(key, defaultValue);
}

QByteArray SettingsStore::byteArrayValue(const QString& key, const QByteArray& defaultValue) const {
  return m_backend->byteArrayValue(key, defaultValue);
}

QSize SettingsStore::sizeValue(const QString& key, const QSize& defaultValue) const {
  return m_backend->sizeValue(key, defaultValue);
}

QSizeF SettingsStore::sizeFValue(const QString& key, const QSizeF& defaultValue) const {
  return m_backend->sizeFValue(key, defaultValue);
}

bool SettingsStore::contains(const QString& key) const {
  return m_backend->contains(key);
}

bool SettingsStore::hasError() const {
  return m_backend->hasError();
}

QStringList SettingsStore::stringList(const QString& key) const {
  return m_backend->stringList(key);
}

void SettingsStore::setValue(const QString& key, bool value) {
  m_backend->setValue(key, value);
  sync();
}

void SettingsStore::setValue(const QString& key, int value) {
  m_backend->setValue(key, value);
  sync();
}

void SettingsStore::setValue(const QString& key, double value) {
  m_backend->setValue(key, value);
  sync();
}

void SettingsStore::setValue(const QString& key, const QString& value) {
  m_backend->setValue(key, value);
  sync();
}

void SettingsStore::setValue(const QString& key, const QByteArray& value) {
  m_backend->setValue(key, value);
  sync();
}

void SettingsStore::setValue(const QString& key, const QSize& value) {
  m_backend->setValue(key, value);
  sync();
}

void SettingsStore::setValue(const QString& key, const QSizeF& value) {
  m_backend->setValue(key, value);
  sync();
}

void SettingsStore::setStringList(const QString& key, const QStringList& values) {
  m_backend->setStringList(key, values);
  sync();
}

void SettingsStore::remove(const QString& key) {
  m_backend->remove(key);
  sync();
}

void SettingsStore::sync() {
  const QString error = m_backend->sync();
  if (!error.isEmpty()) {
    reportWriteError(m_filePath, error);
  }
}
