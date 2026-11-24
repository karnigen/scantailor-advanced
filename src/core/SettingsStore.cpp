// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SettingsStore.h"

#include <config.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QMetaType>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QVariant>

#include <toml++/toml.h>

#include <atomic>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr int kTomlSchemaVersion = 1;

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

QStringList keyParts(const QString& key) {
  return key.split(QLatin1Char('/'), Qt::SkipEmptyParts);
}

const toml::node* findNode(const toml::table& document, const QString& key) {
  const QStringList parts = keyParts(key);
  if (parts.isEmpty()) {
    return nullptr;
  }
  const toml::table* table = &document;
  for (qsizetype i = 0; i < parts.size(); ++i) {
    const toml::node* node = table->get(parts.at(i).toStdString());
    if (!node) {
      return nullptr;
    }
    if (i == parts.size() - 1) {
      return node;
    }
    table = node->as_table();
    if (!table) {
      return nullptr;
    }
  }
  return nullptr;
}

using TomlMutation = std::function<bool(toml::table&, QString*)>;

template <typename T>
bool assignNode(toml::table& document, const QString& key, const T& value, QString* errorMessage) {
  const QStringList parts = keyParts(key);
  if (parts.isEmpty()) {
    return false;
  }
  toml::table* table = &document;
  for (qsizetype i = 0; i < parts.size() - 1; ++i) {
    const std::string part = parts.at(i).toStdString();
    toml::node* node = table->get(part);
    if (!node) {
      table->insert_or_assign(part, toml::table{});
      node = table->get(part);
    }
    table = node->as_table();
    if (!table) {
      *errorMessage = QObject::tr("Cannot update TOML key '%1': '%2' is not a table.").arg(key, parts.at(i));
      return false;
    }
    if (parts.size() > 2 && parts.at(0) == QStringLiteral("CollapsibleGroupBox") && i == 1) {
      table->is_inline(true);
    }
  }
  table->insert_or_assign(parts.back().toStdString(), value);
  return true;
}

bool removeNode(toml::table& document, const QString& key, QString* errorMessage) {
  const QStringList parts = keyParts(key);
  if (parts.isEmpty()) {
    return true;
  }
  toml::table* table = &document;
  for (qsizetype i = 0; i < parts.size() - 1; ++i) {
    toml::node* node = table->get(parts.at(i).toStdString());
    if (!node) {
      return true;
    }
    table = node->as_table();
    if (!table) {
      *errorMessage = QObject::tr("Cannot remove TOML key '%1': '%2' is not a table.").arg(key, parts.at(i));
      return false;
    }
  }
  table->erase(parts.back().toStdString());
  return true;
}

struct TomlState {
  explicit TomlState(QString path) : filePath(std::move(path)) {
    if (!QFileInfo::exists(filePath)) {
      document.insert_or_assign("schema_version", kTomlSchemaVersion);
      return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
      error = QObject::tr("Unable to read the TOML settings file: %1").arg(file.errorString());
      return;
    }
    const QByteArray contents = file.readAll();
    try {
      document = toml::parse(std::string_view(contents.constData(), static_cast<std::size_t>(contents.size())));
    } catch (const toml::parse_error& parseError) {
      error = QObject::tr("Invalid TOML in %1: %2").arg(filePath, QString::fromUtf8(parseError.what()));
      return;
    }

    const auto schemaVersion = document.get("schema_version");
    const auto version = schemaVersion && schemaVersion->type() == toml::node_type::integer
                             ? schemaVersion->value<std::int64_t>()
                             : std::nullopt;
    if (!version || *version != kTomlSchemaVersion) {
      error = QObject::tr("Unsupported or missing configuration schema_version in %1 (expected %2).")
                  .arg(filePath)
                  .arg(kTomlSchemaVersion);
      return;
    }

    static constexpr std::string_view expectedTables[]{"settings", "mainWindow", "main_window",
                                                       "project",  "window",     "margins"};
    for (const std::string_view name : expectedTables) {
      const toml::node* node = document.get(name);
      if (node && !node->is_table()) {
        error = QObject::tr("Expected TOML table '%1' in %2.")
                    .arg(QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())), filePath);
        return;
      }
    }
  }

  QString filePath;
  mutable std::mutex mutex;
  toml::table document;
  std::vector<TomlMutation> pendingMutations;
  QString error;
  QString persistenceError;
};

std::shared_ptr<TomlState> g_activeTomlState;

std::shared_ptr<TomlState> stateForPath(const QString& filePath) {
  static std::mutex statesMutex;
  static std::map<QString, std::weak_ptr<TomlState>> states;
  std::lock_guard<std::mutex> lock(statesMutex);
  auto& weakState = states[filePath];
  if (auto state = weakState.lock()) {
    return state;
  }
  auto state = std::make_shared<TomlState>(filePath);
  weakState = state;
  return state;
}

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

  bool hasError() const override { return m_settings.status() != QSettings::NoError || !m_persistenceError.isEmpty(); }

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
    QString error;
    switch (m_settings.status()) {
      case QSettings::NoError:
        break;
      case QSettings::AccessError:
        error = QObject::tr("access to the settings file was denied");
        break;
      case QSettings::FormatError:
        error = QObject::tr("the settings file has an invalid format");
        break;
    }
    if (error.isEmpty() && m_settings.status() != QSettings::NoError) {
      error = QObject::tr("the settings file could not be saved");
    }
    m_persistenceError = error;
    return error;
  }

  QSettings& settings() { return m_settings; }

 private:
  mutable QSettings m_settings;
  QString m_persistenceError;
};

class TomlBackend final : public SettingsStore::Backend {
 public:
  explicit TomlBackend(const QString& filePath) : m_state(stateForPath(filePath)) {}

  bool boolValue(const QString& key, bool defaultValue) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    const toml::node* node = findNode(m_state->document, key);
    if (!node) {
      return defaultValue;
    }
    const auto value = node->type() == toml::node_type::boolean ? node->value<bool>() : std::nullopt;
    if (!value) {
      recordTypeError(key, QStringLiteral("a boolean"));
      return defaultValue;
    }
    return *value;
  }

  int intValue(const QString& key, int defaultValue, bool* ok) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    const toml::node* node = findNode(m_state->document, key);
    if (!node) {
      if (ok) {
        *ok = true;
      }
      return defaultValue;
    }
    const auto value = node->type() == toml::node_type::integer ? node->value<std::int64_t>() : std::nullopt;
    if (!value || *value < std::numeric_limits<int>::min() || *value > std::numeric_limits<int>::max()) {
      if (ok) {
        *ok = false;
      }
      recordTypeError(key, QStringLiteral("a 32-bit integer"));
      return defaultValue;
    }
    if (ok) {
      *ok = true;
    }
    return static_cast<int>(*value);
  }

  double doubleValue(const QString& key, double defaultValue) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    const toml::node* node = findNode(m_state->document, key);
    if (!node) {
      return defaultValue;
    }
    double value = 0.0;
    if (node->type() == toml::node_type::floating_point) {
      value = *node->value<double>();
    } else if (node->type() == toml::node_type::integer) {
      value = static_cast<double>(*node->value<std::int64_t>());
    } else {
      recordTypeError(key, QStringLiteral("a finite number"));
      return defaultValue;
    }
    if (!std::isfinite(value)) {
      recordTypeError(key, QStringLiteral("a finite number"));
      return defaultValue;
    }
    return value;
  }

  QString stringValue(const QString& key, const QString& defaultValue) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    const toml::node* node = findNode(m_state->document, key);
    if (!node) {
      return defaultValue;
    }
    const auto value = node->type() == toml::node_type::string ? node->value<std::string>() : std::nullopt;
    if (!value) {
      recordTypeError(key, QStringLiteral("a string"));
      return defaultValue;
    }
    return QString::fromUtf8(value->data(), static_cast<qsizetype>(value->size()));
  }

  QByteArray byteArrayValue(const QString& key, const QByteArray& defaultValue) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    const toml::node* node = findNode(m_state->document, key);
    if (!node) {
      return defaultValue;
    }
    const auto value = node->type() == toml::node_type::string ? node->value<std::string>() : std::nullopt;
    if (!value) {
      recordByteArrayTypeError(key);
      return defaultValue;
    }
    const QByteArray hex = QByteArray::fromStdString(*value);
    const QByteArray bytes = QByteArray::fromHex(hex);
    if (bytes.toHex() != hex.toLower()) {
      recordByteArrayTypeError(key);
      return defaultValue;
    }
    return bytes;
  }

  QSize sizeValue(const QString& key, const QSize& defaultValue) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    const toml::node* node = findNode(m_state->document, key);
    if (!node) {
      return defaultValue;
    }
    const toml::array* values = node->as_array();
    if (!values || values->size() != 2) {
      recordTypeError(key, QStringLiteral("a two-integer array"));
      return defaultValue;
    }
    const toml::node* widthNode = values->get(0);
    const toml::node* heightNode = values->get(1);
    const auto width
        = widthNode && widthNode->type() == toml::node_type::integer ? widthNode->value<std::int64_t>() : std::nullopt;
    const auto height = heightNode && heightNode->type() == toml::node_type::integer ? heightNode->value<std::int64_t>()
                                                                                     : std::nullopt;
    if (!width || !height || *width <= 0 || *height <= 0 || *width < std::numeric_limits<int>::min()
        || *width > std::numeric_limits<int>::max() || *height < std::numeric_limits<int>::min()
        || *height > std::numeric_limits<int>::max()) {
      recordTypeError(key, QStringLiteral("a two-positive-integer array"));
      return defaultValue;
    }
    return QSize(static_cast<int>(*width), static_cast<int>(*height));
  }

  QSizeF sizeFValue(const QString& key, const QSizeF& defaultValue) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    const toml::node* node = findNode(m_state->document, key);
    if (!node) {
      return defaultValue;
    }
    const toml::array* values = node->as_array();
    if (!values || values->size() != 2) {
      recordTypeError(key, QStringLiteral("a two-number array"));
      return defaultValue;
    }
    const auto width = numericValue(values->get(0));
    const auto height = numericValue(values->get(1));
    if (!width || !height || *width <= 0.0 || *height <= 0.0) {
      recordTypeError(key, QStringLiteral("a two-positive-finite-number array"));
      return defaultValue;
    }
    return QSizeF(*width, *height);
  }

  bool contains(const QString& key) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return findNode(m_state->document, key) != nullptr;
  }

  bool hasError() const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return !m_state->error.isEmpty() || !m_state->persistenceError.isEmpty();
  }

  QString errorString() const {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return m_state->error;
  }

  QStringList stringList(const QString& key) const override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    const toml::node* node = findNode(m_state->document, key);
    if (!node) {
      return {};
    }
    const toml::array* values = node->as_array();
    if (!values) {
      recordTypeError(key, QStringLiteral("an array of strings"));
      return {};
    }
    QStringList result;
    for (const toml::node& item : *values) {
      const auto value = item.type() == toml::node_type::string ? item.value<std::string>() : std::nullopt;
      if (!value) {
        recordTypeError(key, QStringLiteral("an array of strings"));
        return {};
      }
      result.push_back(QString::fromUtf8(value->data(), static_cast<qsizetype>(value->size())));
    }
    return result;
  }

  void setValue(const QString& key, bool value) override { setNode(key, value); }

  void setValue(const QString& key, int value) override { setNode(key, static_cast<std::int64_t>(value)); }

  void setValue(const QString& key, double value) override { setNode(key, value); }

  void setValue(const QString& key, const QString& value) override { setNode(key, value.toStdString()); }

  void setValue(const QString& key, const QByteArray& value) override { setNode(key, value.toHex().toStdString()); }

  void setValue(const QString& key, const QSize& value) override {
    toml::array values;
    values.push_back(value.width());
    values.push_back(value.height());
    setNode(key, std::move(values));
  }

  void setValue(const QString& key, const QSizeF& value) override {
    toml::array values;
    values.push_back(value.width());
    values.push_back(value.height());
    setNode(key, std::move(values));
  }

  void setStringList(const QString& key, const QStringList& values) override {
    toml::array items;
    for (const QString& value : values) {
      items.push_back(value.toStdString());
    }
    setNode(key, std::move(items));
  }

  void remove(const QString& key) override {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    if (!m_state->error.isEmpty()) {
      return;
    }
    TomlMutation mutation
        = [key](toml::table& document, QString* errorMessage) { return removeNode(document, key, errorMessage); };
    QString mutationError;
    if (!mutation(m_state->document, &mutationError)) {
      recordError(mutationError);
      return;
    }
    m_state->pendingMutations.push_back(std::move(mutation));
  }

  QString sync() override { return sync(false); }

  QString sync(bool forceWrite) {
    {
      std::lock_guard<std::mutex> lock(m_state->mutex);
      if (!m_state->error.isEmpty()) {
        return m_state->error;
      }
      if (!forceWrite && m_state->pendingMutations.empty() && !QFileInfo::exists(m_state->filePath)) {
        m_state->persistenceError.clear();
        return {};
      }
    }

    QLockFile processLock(m_state->filePath + QStringLiteral(".lock"));
    processLock.setStaleLockTime(30000);
    if (!processLock.tryLock(10000)) {
      const QString error = QObject::tr("unable to lock the TOML settings file: %1").arg(m_state->filePath);
      std::lock_guard<std::mutex> lock(m_state->mutex);
      m_state->persistenceError = error;
      return error;
    }

    std::lock_guard<std::mutex> lock(m_state->mutex);
    if (!m_state->error.isEmpty()) {
      return m_state->error;
    }
    if (!forceWrite && m_state->pendingMutations.empty() && !QFileInfo::exists(m_state->filePath)) {
      m_state->persistenceError.clear();
      return {};
    }
    TomlState latest(m_state->filePath);
    if (!latest.error.isEmpty()) {
      m_state->persistenceError = latest.error;
      return m_state->persistenceError;
    }
    if (m_state->pendingMutations.empty() && !forceWrite) {
      m_state->document = std::move(latest.document);
      m_state->persistenceError.clear();
      return {};
    }
    for (const TomlMutation& mutation : m_state->pendingMutations) {
      QString mutationError;
      if (!mutation(latest.document, &mutationError)) {
        m_state->persistenceError = mutationError;
        return m_state->persistenceError;
      }
    }
    std::ostringstream output;
    output << latest.document;
    const std::string serialized = output.str();
    QSaveFile file(m_state->filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      m_state->persistenceError = QObject::tr("unable to open the TOML file for writing: %1").arg(file.errorString());
      return m_state->persistenceError;
    }
    if (file.write(serialized.data(), static_cast<qint64>(serialized.size()))
        != static_cast<qint64>(serialized.size())) {
      const QString error = file.errorString();
      file.cancelWriting();
      m_state->persistenceError = QObject::tr("unable to write the TOML file: %1").arg(error);
      return m_state->persistenceError;
    }
    if (!file.commit()) {
      m_state->persistenceError = QObject::tr("unable to atomically replace the TOML file: %1").arg(file.errorString());
      return m_state->persistenceError;
    }
    m_state->document = std::move(latest.document);
    m_state->pendingMutations.clear();
    m_state->persistenceError.clear();
    return {};
  }

  std::string serializedDocument() const {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    std::ostringstream output;
    output << m_state->document;
    return output.str();
  }

 private:
  static std::optional<double> numericValue(const toml::node* node) {
    if (!node) {
      return std::nullopt;
    }
    if (node->type() == toml::node_type::floating_point) {
      const double value = *node->value<double>();
      return std::isfinite(value) ? std::optional<double>(value) : std::nullopt;
    }
    if (node->type() == toml::node_type::integer) {
      return static_cast<double>(*node->value<std::int64_t>());
    }
    return std::nullopt;
  }

  void recordTypeError(const QString& key, const QString& expected) const {
    recordError(QObject::tr("Invalid type for configuration key '%1' in %2; expected %3.")
                    .arg(key, m_state->filePath, expected));
  }

  void recordByteArrayTypeError(const QString& key) const {
    if (key == QStringLiteral("mainWindow/nonMaximizedGeometry")) {
      qWarning().noquote()
          << QObject::tr("Invalid window geometry in %1; the default window size will be used.").arg(m_state->filePath);
      return;
    }
    recordTypeError(key, QStringLiteral("a hexadecimal string"));
  }

  void recordError(const QString& error) const {
    if (!error.isEmpty() && m_state->error.isEmpty()) {
      m_state->error = error;
      qWarning().noquote() << m_state->error;
    }
  }

  template <typename T>
  void setNode(const QString& key, T&& value) {
    using Value = std::decay_t<T>;
    Value storedValue(std::forward<T>(value));
    TomlMutation mutation = [key, storedValue](toml::table& document, QString* errorMessage) {
      return assignNode(document, key, storedValue, errorMessage);
    };

    std::lock_guard<std::mutex> lock(m_state->mutex);
    if (!m_state->error.isEmpty()) {
      return;
    }
    QString mutationError;
    if (!mutation(m_state->document, &mutationError)) {
      recordError(mutationError);
      return;
    }
    m_state->pendingMutations.push_back(std::move(mutation));
  }

  std::shared_ptr<TomlState> m_state;
};

bool isBooleanKey(const QString& key) {
  static const QStringList keys{
      QStringLiteral("settings/enable_opengl"),
      QStringLiteral("settings/auto_save_project"),
      QStringLiteral("settings/black_on_white_detection"),
      QStringLiteral("settings/black_on_white_detection_at_output"),
      QStringLiteral("settings/highlight_deviation"),
      QStringLiteral("settings/single_column_thumbnail_display"),
      QStringLiteral("settings/selection_canceling_question"),
      QStringLiteral("settings/output_show_guides"),
      QStringLiteral("mainWindow/maximized"),
      QStringLiteral("mainWindow/keepCurrentPageInView"),
      QStringLiteral("margins/topBottomLinked"),
      QStringLiteral("margins/leftRightLinked"),
  };
  return keys.contains(key) || key.endsWith(QStringLiteral("/checked")) || key.endsWith(QStringLiteral("/collapsed"));
}

bool isIntegerKey(const QString& key) {
  static const QStringList keys{
      QStringLiteral("settings/bw_compression"),
      QStringLiteral("settings/color_compression"),
      QStringLiteral("settings/default_zone_creation_mode"),
      QStringLiteral("settings/image_allocation_limit_mb"),
      QStringLiteral("settings/batch_processing_threads"),
  };
  return keys.contains(key);
}

bool isDoubleKey(const QString& key) {
  static const QStringList keys{
      QStringLiteral("settings/deskew_deviation_coef"),
      QStringLiteral("settings/deskew_deviation_threshold"),
      QStringLiteral("settings/select_content_deviation_coef"),
      QStringLiteral("settings/select_content_deviation_threshold"),
      QStringLiteral("settings/margins_deviation_coef"),
      QStringLiteral("settings/margins_deviation_threshold"),
  };
  return keys.contains(key);
}

bool isSizeKey(const QString& key) {
  return key == QStringLiteral("settings/thumbnail_quality");
}

bool isSizeFKey(const QString& key) {
  return key == QStringLiteral("settings/max_logical_thumb_size");
}

bool isIntegerVariant(const QVariant& value) {
  switch (value.metaType().id()) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
      return true;
    default:
      return false;
  }
}

bool integerValue(const QVariant& value, int* converted) {
  bool conversionSucceeded = false;
  qlonglong integer = 0;
  if (value.metaType().id() == QMetaType::QString) {
    integer = value.toString().trimmed().toLongLong(&conversionSucceeded, 10);
  } else if (isIntegerVariant(value)) {
    integer = value.toLongLong(&conversionSucceeded);
  }
  if (!conversionSucceeded || integer < std::numeric_limits<int>::min() || integer > std::numeric_limits<int>::max()) {
    return false;
  }
  *converted = static_cast<int>(integer);
  return true;
}

bool finiteNumberValue(const QVariant& value, double* converted) {
  const int type = value.metaType().id();
  if (type != QMetaType::QString && type != QMetaType::Double && type != QMetaType::Float && !isIntegerVariant(value)) {
    return false;
  }
  bool conversionSucceeded = false;
  const double number = value.toDouble(&conversionSucceeded);
  if (!conversionSucceeded || !std::isfinite(number)) {
    return false;
  }
  *converted = number;
  return true;
}

bool convertIniValue(TomlBackend& destination, const QString& key, const QVariant& value, QString* error) {
  bool conversionSucceeded = false;
  if (isBooleanKey(key)) {
    const bool converted = value.toBool();
    if (value.metaType().id() == QMetaType::QString) {
      const QString text = value.toString().trimmed().toLower();
      if (text != QStringLiteral("true") && text != QStringLiteral("false") && text != QStringLiteral("1")
          && text != QStringLiteral("0")) {
        conversionSucceeded = false;
      } else {
        conversionSucceeded = true;
      }
    } else {
      conversionSucceeded = value.canConvert<bool>();
    }
    if (!conversionSucceeded) {
      *error = QObject::tr("Invalid boolean value for legacy INI key '%1'.").arg(key);
      return false;
    }
    destination.setValue(key, converted);
  } else if (isIntegerKey(key)) {
    int converted = 0;
    if (!integerValue(value, &converted)) {
      *error = QObject::tr("Invalid integer value for legacy INI key '%1'.").arg(key);
      return false;
    }
    destination.setValue(key, converted);
  } else if (isDoubleKey(key)) {
    double converted = 0.0;
    if (!finiteNumberValue(value, &converted)) {
      *error = QObject::tr("Invalid finite number value for legacy INI key '%1'.").arg(key);
      return false;
    }
    destination.setValue(key, converted);
  } else if (isSizeKey(key)) {
    if (value.metaType().id() != QMetaType::QSize) {
      *error = QObject::tr("Invalid size value for legacy INI key '%1'.").arg(key);
      return false;
    }
    const QSize converted = value.toSize();
    if (converted.width() <= 0 || converted.height() <= 0) {
      *error = QObject::tr("Invalid size value for legacy INI key '%1'.").arg(key);
      return false;
    }
    destination.setValue(key, converted);
  } else if (isSizeFKey(key)) {
    if (value.metaType().id() != QMetaType::QSizeF) {
      *error = QObject::tr("Invalid size value for legacy INI key '%1'.").arg(key);
      return false;
    }
    const QSizeF converted = value.toSizeF();
    if (!std::isfinite(converted.width()) || !std::isfinite(converted.height()) || converted.width() <= 0.0
        || converted.height() <= 0.0) {
      *error = QObject::tr("Invalid size value for legacy INI key '%1'.").arg(key);
      return false;
    }
    destination.setValue(key, converted);
  } else {
    switch (value.metaType().id()) {
      case QMetaType::Bool:
        destination.setValue(key, value.toBool());
        break;
      case QMetaType::Int:
      case QMetaType::UInt:
      case QMetaType::LongLong:
      case QMetaType::ULongLong: {
        const qlonglong converted = value.toLongLong(&conversionSucceeded);
        if (!conversionSucceeded || converted < std::numeric_limits<int>::min()
            || converted > std::numeric_limits<int>::max()) {
          *error = QObject::tr("Unsupported integer value for legacy INI key '%1'.").arg(key);
          return false;
        }
        destination.setValue(key, static_cast<int>(converted));
        break;
      }
      case QMetaType::Double:
        destination.setValue(key, value.toDouble());
        break;
      case QMetaType::QString:
        destination.setValue(key, value.toString());
        break;
      case QMetaType::QByteArray:
        destination.setValue(key, value.toByteArray());
        break;
      case QMetaType::QSize:
        destination.setValue(key, value.toSize());
        break;
      case QMetaType::QSizeF:
        destination.setValue(key, value.toSizeF());
        break;
      default:
        *error = QObject::tr("Cannot safely migrate the legacy INI value for key '%1' (type %2).")
                     .arg(key, QString::fromLatin1(value.metaType().name()));
        return false;
    }
  }
  return true;
}

bool validateTomlValues(TomlBackend& settings) {
  static const QStringList booleanKeys{
      QStringLiteral("settings/enable_opengl"),
      QStringLiteral("settings/auto_save_project"),
      QStringLiteral("settings/black_on_white_detection"),
      QStringLiteral("settings/black_on_white_detection_at_output"),
      QStringLiteral("settings/highlight_deviation"),
      QStringLiteral("settings/single_column_thumbnail_display"),
      QStringLiteral("settings/selection_canceling_question"),
      QStringLiteral("settings/output_show_guides"),
      QStringLiteral("mainWindow/maximized"),
      QStringLiteral("mainWindow/keepCurrentPageInView"),
      QStringLiteral("margins/topBottomLinked"),
      QStringLiteral("margins/leftRightLinked"),
  };
  static const QStringList integerKeys{
      QStringLiteral("settings/bw_compression"),
      QStringLiteral("settings/color_compression"),
      QStringLiteral("settings/default_zone_creation_mode"),
      QStringLiteral("settings/image_allocation_limit_mb"),
      QStringLiteral("settings/batch_processing_threads"),
  };
  static const QStringList doubleKeys{
      QStringLiteral("settings/deskew_deviation_coef"),
      QStringLiteral("settings/deskew_deviation_threshold"),
      QStringLiteral("settings/select_content_deviation_coef"),
      QStringLiteral("settings/select_content_deviation_threshold"),
      QStringLiteral("settings/margins_deviation_coef"),
      QStringLiteral("settings/margins_deviation_threshold"),
  };
  static const QStringList stringKeys{
      QStringLiteral("settings/color_scheme"),
      QStringLiteral("settings/language"),
      QStringLiteral("settings/units"),
      QStringLiteral("settings/current_profile"),
      QStringLiteral("main_window/external_alarm_cmd"),
      QStringLiteral("project/lastDir"),
      QStringLiteral("lastInputDir"),
  };

  for (const QString& key : booleanKeys) {
    settings.boolValue(key, false);
  }
  for (const QString& key : integerKeys) {
    settings.intValue(key, 0, nullptr);
  }
  for (const QString& key : doubleKeys) {
    settings.doubleValue(key, 0.0);
  }
  for (const QString& key : stringKeys) {
    settings.stringValue(key, QString());
  }
  settings.sizeValue(QStringLiteral("settings/thumbnail_quality"), QSize());
  settings.sizeFValue(QStringLiteral("settings/max_logical_thumb_size"), QSizeF());
  settings.byteArrayValue(QStringLiteral("mainWindow/nonMaximizedGeometry"), QByteArray());
  settings.stringList(QStringLiteral("project/recent"));
  return !settings.hasError();
}

bool validateTomlCandidate(TomlBackend& settings, QString* errorMessage) {
  const std::string serialized = settings.serializedDocument();
  QTemporaryFile candidate;
  if (!candidate.open()) {
    *errorMessage = QObject::tr("Unable to create a temporary file to validate the migrated TOML: %1")
                        .arg(candidate.errorString());
    return false;
  }
  if (candidate.write(serialized.data(), static_cast<qint64>(serialized.size()))
      != static_cast<qint64>(serialized.size())) {
    *errorMessage = QObject::tr("Unable to write the temporary migrated TOML: %1").arg(candidate.errorString());
    return false;
  }
  candidate.close();

  TomlBackend parsedCandidate(candidate.fileName());
  if (parsedCandidate.hasError() || !validateTomlValues(parsedCandidate)) {
    *errorMessage = QObject::tr("The converted TOML settings are invalid: %1").arg(parsedCandidate.errorString());
    return false;
  }
  return true;
}

bool migrateIniToToml(const QString& iniPath, const QString& tomlPath, bool persist, QString* errorMessage) {
  IniBackend source(iniPath);
  QSettings& ini = source.settings();
  ini.sync();
  if (source.hasError()) {
    *errorMessage = QObject::tr("Unable to read the legacy INI settings file: %1").arg(iniPath);
    return false;
  }

  TomlBackend destination(tomlPath);
  if (destination.hasError()) {
    *errorMessage = QObject::tr("Unable to initialize the TOML settings file: %1").arg(tomlPath);
    return false;
  }

  const QStringList keys = ini.allKeys();
  if (source.hasError()) {
    *errorMessage = QObject::tr(
                        "Unable to parse the legacy INI settings file: %1. The original INI file was left "
                        "unchanged.")
                        .arg(iniPath);
    return false;
  }

  for (const QString& key : keys) {
    if (key.startsWith(QStringLiteral("project/recent/"))) {
      continue;
    }
    const QVariant value = ini.value(key);
    if (source.hasError()) {
      *errorMessage = QObject::tr(
                          "Unable to parse the legacy INI settings file: %1. The original INI file was left "
                          "unchanged.")
                          .arg(iniPath);
      return false;
    }
    if (!convertIniValue(destination, key, value, errorMessage)) {
      *errorMessage += QObject::tr(" The original INI file was left unchanged.");
      return false;
    }
  }

  if (ini.contains(QStringLiteral("project/recent/size"))) {
    const QStringList recentProjects = source.stringList(QStringLiteral("project/recent"));
    if (source.hasError()) {
      *errorMessage = QObject::tr(
                          "Unable to parse the legacy INI settings file: %1. The original INI file was left "
                          "unchanged.")
                          .arg(iniPath);
      return false;
    }
    destination.setStringList(QStringLiteral("project/recent"), recentProjects);
  }
  if (source.hasError()) {
    *errorMessage = QObject::tr(
                        "Unable to parse the legacy INI settings file: %1. The original INI file was left "
                        "unchanged.")
                        .arg(iniPath);
    return false;
  }
  if (destination.hasError()) {
    *errorMessage
        = QObject::tr("Unable to convert values from %1. The original INI file was left unchanged.").arg(iniPath);
    return false;
  }
  if (!validateTomlCandidate(destination, errorMessage)) {
    *errorMessage += QObject::tr(" The original INI file was left unchanged.");
    return false;
  }

  if (!persist) {
    return true;
  }

  const QString writeError = destination.sync(true);
  if (!writeError.isEmpty()) {
    *errorMessage = QObject::tr("Unable to migrate %1 to %2: %3. The original INI file was left unchanged.")
                        .arg(iniPath, tomlPath, writeError);
    return false;
  }
  return true;
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

  const QString tomlPath = QDir(directory).filePath(QStringLiteral("config.toml"));
  const QString iniPath = QDir(directory).filePath(QStringLiteral("config.ini"));
  QString initializationWarning = writable ? QString() : probeError;

  if (QFileInfo::exists(tomlPath)) {
    const std::shared_ptr<TomlState> configState = stateForPath(tomlPath);
    TomlBackend toml(tomlPath);
    if (toml.hasError() || !validateTomlValues(toml)) {
      if (errorMessage) {
        *errorMessage = toml.errorString();
      }
      return false;
    }
    g_activeTomlState = configState;
  } else {
    std::unique_ptr<QLockFile> migrationLock;
    if (writable) {
      migrationLock = std::make_unique<QLockFile>(tomlPath + QStringLiteral(".migration.lock"));
      migrationLock->setStaleLockTime(30000);
      if (!migrationLock->tryLock(10000)) {
        if (errorMessage) {
          *errorMessage = QObject::tr("Unable to lock the configuration migration: %1").arg(tomlPath);
        }
        return false;
      }
    }

    if (QFileInfo::exists(tomlPath)) {
      const std::shared_ptr<TomlState> configState = stateForPath(tomlPath);
      TomlBackend toml(tomlPath);
      if (toml.hasError() || !validateTomlValues(toml)) {
        if (errorMessage) {
          *errorMessage = toml.errorString();
        }
        return false;
      }
      g_activeTomlState = configState;
    } else {
      if (writable && preserveExistingSettings && !QFileInfo::exists(tomlPath) && !QFileInfo::exists(iniPath)) {
        const QString legacyFilePath = IniBackend::legacyFilePath();
        if (QFileInfo::exists(legacyFilePath) && legacyFilePath != iniPath && !QFile::copy(legacyFilePath, iniPath)) {
          initializationWarning
              = QObject::tr("Unable to copy the existing settings file from %1 to %2.").arg(legacyFilePath, iniPath);
        }
      }

      QString migrationIniPath;
      if (QFileInfo::exists(iniPath)) {
        migrationIniPath = iniPath;
      } else if (!writable && preserveExistingSettings) {
        const QString legacyFilePath = IniBackend::legacyFilePath();
        if (QFileInfo::exists(legacyFilePath)) {
          migrationIniPath = legacyFilePath;
        }
      }

      if (migrationIniPath.isEmpty()) {
        g_activeTomlState = stateForPath(tomlPath);
      } else {
        const std::shared_ptr<TomlState> configState = stateForPath(tomlPath);
        QString migrationError;
        if (!migrateIniToToml(migrationIniPath, tomlPath, writable, &migrationError)) {
          if (errorMessage) {
            *errorMessage = migrationError;
          }
          return false;
        }
        g_activeTomlState = configState;
        if (writable && migrationIniPath == iniPath) {
          const QString backupPath = iniPath + QStringLiteral(".bak");
          if (!QFile::rename(iniPath, backupPath)) {
            initializationWarning
                += QObject::tr(
                       " Settings were migrated to TOML, but the original INI could not be renamed to "
                       "%1; both files were left in place.")
                       .arg(backupPath);
          }
        } else if (!writable) {
          initializationWarning += QObject::tr(
              " Legacy INI settings were loaded in memory; changes cannot be saved until the configuration directory "
              "is writable.");
        }
      }
    }
  }

  g_configDirectory = directory;
  g_configFilePath = tomlPath;
  g_configDirectoryExplicit = explicitDirectory;
  g_configDirectoryWritable = writable;
  g_initialized = true;
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
  m_backend = std::make_unique<TomlBackend>(m_filePath);
}

SettingsStore::SettingsStore(const QString& filePath)
    : m_filePath(filePath), m_backend(std::make_unique<TomlBackend>(filePath)) {}

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
