// Copyright (C) 2019  Joseph Artsimovich <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ConfigurationDialog.h"

#include <core/SettingsStore.h>

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

ConfigurationDialog::ConfigurationDialog(QWidget* parent)
    : QDialog(parent),
      m_pathLabel(new QLabel(this)),
      m_profilesPathLabel(new QLabel(this)),
      m_statusLabel(new QLabel(this)),
      m_contents(new QPlainTextEdit(this)) {
  setWindowTitle(tr("Configuration"));
  resize(700, 500);

  auto* layout = new QVBoxLayout(this);
  const QString mode = SettingsStore::configDirectoryExplicit() ? tr("Custom directory") : tr("Platform default");
  auto* modeLabel = new QLabel(tr("Location: %1").arg(mode), this);
  modeLabel->setToolTip(tr("Shows whether the configuration directory is custom or platform default."));
  modeLabel->setObjectName(QStringLiteral("configurationMode"));
  layout->addWidget(modeLabel);
  m_pathLabel->setObjectName(QStringLiteral("configurationFilePath"));
  m_pathLabel->setToolTip(tr("Full path to the configuration file."));
  m_profilesPathLabel->setObjectName(QStringLiteral("configurationProfilesPath"));
  m_profilesPathLabel->setToolTip(tr("Full path to the folder containing parameter profiles."));
  m_statusLabel->setObjectName(QStringLiteral("configurationDirectoryStatus"));
  m_statusLabel->setToolTip(tr("Shows whether the configuration directory can be written to."));
  layout->addWidget(m_pathLabel);
  layout->addWidget(m_profilesPathLabel);
  layout->addWidget(m_statusLabel);
  layout->addWidget(
      new QLabel(tr("An existing config.ini is migrated once. After TOML is saved, the application attempts to rename "
                    "it to config.ini.bak; if that fails or the backup already exists, config.ini remains in place. "
                    "Unknown TOML keys are retained."),
                 this));

  m_contents->setReadOnly(true);
  m_contents->setObjectName(QStringLiteral("configurationContents"));
  m_contents->setToolTip(tr("Read-only contents of the configuration file."));
  m_contents->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  layout->addWidget(m_contents);

  auto* buttonsLayout = new QHBoxLayout;
  auto* copyButton = new QPushButton(tr("Copy path"), this);
  copyButton->setObjectName(QStringLiteral("configurationCopyPathButton"));
  copyButton->setToolTip(tr("Copy the configuration file path to the clipboard."));
  auto* openButton = new QPushButton(tr("Open folder"), this);
  openButton->setToolTip(tr("Open the configuration folder in the file manager."));
  auto* reloadButton = new QPushButton(tr("Reload"), this);
  reloadButton->setObjectName(QStringLiteral("configurationReloadButton"));
  reloadButton->setToolTip(tr("Reload the configuration file contents from disk."));
  auto* closeButton = new QPushButton(tr("Close"), this);
  closeButton->setToolTip(tr("Close this dialog."));
  buttonsLayout->addWidget(copyButton);
  buttonsLayout->addWidget(openButton);
  buttonsLayout->addWidget(reloadButton);
  buttonsLayout->addStretch();
  buttonsLayout->addWidget(closeButton);
  layout->addLayout(buttonsLayout);

  connect(copyButton, &QPushButton::clicked, this,
          [this]() { QApplication::clipboard()->setText(SettingsStore::configFilePath()); });
  connect(openButton, &QPushButton::clicked, this, [this]() {
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(SettingsStore::configDirectory()))) {
      QMessageBox::warning(this, tr("Configuration"), tr("Unable to open the configuration folder."));
    }
  });
  connect(reloadButton, &QPushButton::clicked, this, &ConfigurationDialog::reload);
  connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
  reload();
}

void ConfigurationDialog::reload() {
  m_pathLabel->setText(tr("Settings file: %1").arg(SettingsStore::configFilePath()));
  m_profilesPathLabel->setText(tr("Profiles folder: %1").arg(SettingsStore::profilesDirectory()));
  m_statusLabel->setText(SettingsStore::configDirectoryWritable() ? tr("Configuration directory is writable.")
                                                                  : tr("Configuration directory is read-only."));

  QFile file(SettingsStore::configFilePath());
  if (!file.exists()) {
    m_contents->setPlainText(tr("The settings file has not been created yet."));
    return;
  }
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_contents->setPlainText(tr("Unable to read the settings file: %1").arg(file.errorString()));
    return;
  }
  m_contents->setPlainText(QString::fromUtf8(file.readAll()));
}
