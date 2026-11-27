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

class ProjectFilesDialog;
class FixDpiDialog;
class QWidget;

class ProjectCreationContext : public QObject {
  Q_OBJECT
  DECLARE_NON_COPYABLE(ProjectCreationContext)

 public:
  explicit ProjectCreationContext(QWidget* parent, std::filesystem::path initialDirectory = {});

  ~ProjectCreationContext() override;

  const std::vector<ImageFileInfo>& files() const { return m_files; }

  const std::filesystem::path& outDir() const { return m_outDir; }

  Qt::LayoutDirection layoutDirection() const { return m_layoutDirection; }

 signals:

  void done(ProjectCreationContext* context);

 private slots:

  void projectFilesSubmitted();

  void projectFilesDialogDestroyed();

  void fixedDpiSubmitted();

  void fixDpiDialogDestroyed();

 private:
  void showProjectFilesDialog();

  void showFixDpiDialog();

  QPointer<ProjectFilesDialog> m_projectFilesDialog;
  QPointer<FixDpiDialog> m_fixDpiDialog;
  std::filesystem::path m_outDir;
  std::vector<ImageFileInfo> m_files;
  Qt::LayoutDirection m_layoutDirection;
  std::filesystem::path m_initialDirectory;
  QWidget* m_parent;
};
