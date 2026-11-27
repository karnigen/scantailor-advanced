// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QObject>
#include <QPointer>
#include <Qt>
#include <filesystem>
#include <vector>

#include "ImageFileInfo.h"
#include "NonCopyable.h"
#include "ProjectReader.h"

class FixDpiDialog;
class QWidget;
class QDomDocument;

class ProjectOpeningContext : public QObject {
  Q_OBJECT
  DECLARE_NON_COPYABLE(ProjectOpeningContext)

 public:
  ProjectOpeningContext(QWidget* parent, std::filesystem::path projectFile, const QDomDocument& doc);

  ~ProjectOpeningContext() override;

  void proceed();

  const std::filesystem::path& projectFile() const { return m_projectFile; }

  ProjectReader* projectReader() { return &m_reader; }

 signals:

  void done(ProjectOpeningContext* context);

 private slots:

  void fixedDpiSubmitted();

  void fixDpiDialogDestroyed();

 private:
  void showFixDpiDialog();

  std::filesystem::path m_projectFile;
  ProjectReader m_reader;
  QPointer<FixDpiDialog> m_fixDpiDialog;
  QWidget* m_parent;
};
