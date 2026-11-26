// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_SETTINGSSTORE_H_
#define SCANTAILOR_CORE_SETTINGSSTORE_H_

#include <QByteArray>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>

#include <filesystem>
#include <functional>
#include <memory>

class SettingsStore {
 public:
  class Backend;

  using WriteErrorHandler = std::function<void(const QString&)>;

  SettingsStore();
  explicit SettingsStore(std::filesystem::path filePath);
  ~SettingsStore();

  SettingsStore(SettingsStore&&) noexcept;
  SettingsStore& operator=(SettingsStore&&) noexcept;

  SettingsStore(const SettingsStore&) = delete;
  SettingsStore& operator=(const SettingsStore&) = delete;

  static bool initialize(const QString& configDirectory,
                         bool explicitDirectory,
                         QString* errorMessage = nullptr,
                         bool preserveExistingSettings = false);
  static std::filesystem::path defaultConfigDirectory();
  static std::filesystem::path configDirectory();
  static std::filesystem::path configFilePath();
  static std::filesystem::path profilesDirectory();
  static bool configDirectoryExplicit();
  static bool configDirectoryWritable();
  static void setWriteErrorHandler(WriteErrorHandler handler);

  bool boolValue(const QString& key, bool defaultValue = false) const;
  int intValue(const QString& key, int defaultValue = 0, bool* ok = nullptr) const;
  double doubleValue(const QString& key, double defaultValue = 0.0) const;
  QString stringValue(const QString& key, const QString& defaultValue = QString()) const;
  QByteArray byteArrayValue(const QString& key, const QByteArray& defaultValue = QByteArray()) const;
  QSize sizeValue(const QString& key, const QSize& defaultValue = QSize()) const;
  QSizeF sizeFValue(const QString& key, const QSizeF& defaultValue = QSizeF()) const;
  bool contains(const QString& key) const;
  bool hasError() const;
  QStringList stringList(const QString& key) const;
  void setValue(const QString& key, bool value);
  void setValue(const QString& key, int value);
  void setValue(const QString& key, double value);
  void setValue(const QString& key, const QString& value);
  void setValue(const QString& key, const QByteArray& value);
  void setValue(const QString& key, const QSize& value);
  void setValue(const QString& key, const QSizeF& value);
  void setStringList(const QString& key, const QStringList& values);
  void remove(const QString& key);
  void sync();

 private:
  std::filesystem::path m_filePath;
  std::unique_ptr<Backend> m_backend;
};

#endif  // SCANTAILOR_CORE_SETTINGSSTORE_H_
