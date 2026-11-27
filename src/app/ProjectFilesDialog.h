// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QDialog>
#include <QSet>
#include <QString>
#include <filesystem>
#include <memory>
#include <vector>

#include "ImageFileInfo.h"
#include "ui_ProjectFilesDialog.h"

class ProjectFilesDialog : public QDialog, private Ui::ProjectFilesDialog {
  Q_OBJECT
 public:
  explicit ProjectFilesDialog(QWidget* parent = nullptr, std::filesystem::path initialDirectory = {});

  ~ProjectFilesDialog() override;

  std::filesystem::path inputDirectory() const;

  std::filesystem::path outputDirectory() const;

  std::vector<ImageFileInfo> inProjectFiles() const;

  bool isRtlLayout() const;

  bool isDpiFixingForced() const;

 private slots:

  static QString sanitizePath(const QString& path);

  void inpDirBrowse();

  void outDirBrowse();

  void inpDirEdited(const QString& text);

  void outDirEdited(const QString& text);

  void addToProject();

  void removeFromProject();

  void onOK();

 private:
  class Item;
  class FileList;
  class SortedFileList;
  class ItemVisualOrdering;

  void setInputDir(const QString& dir, bool autoAddFiles = true);

  void setOutputDir(const QString& dir);

  void startLoadingMetadata();

  void timerEvent(QTimerEvent* event) override;

  void finishLoadingMetadata();

  void setupIcons();

  QSet<QString> m_supportedExtensions;
  std::unique_ptr<FileList> m_offProjectFiles;
  std::unique_ptr<SortedFileList> m_offProjectFilesSorted;
  std::unique_ptr<FileList> m_inProjectFiles;
  std::unique_ptr<SortedFileList> m_inProjectFilesSorted;
  int m_loadTimerId;
  bool m_metadataLoadFailed;
  bool m_autoOutDir;
  std::filesystem::path m_initialDirectory;
};
